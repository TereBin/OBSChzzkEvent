#include "check.hpp"
#include "chzzk-alert-audio.hpp"
#include <map>
#include <string>
#include <cmath>

struct obs_data {
	std::string file;
	bool restartOnActivate = false;
};
struct obs_source {
	int showing = 0;
	int frames = 0;
	float volume = 1;
	float sample = 0;
	bool active = true;
	bool paused = false;
	void *param = nullptr;
	obs_source_audio_capture_t capture = nullptr;
};
static std::map<std::string, obs_source_t *> files;
static int released = 0;
static int created = 0;
obs_data_t *obs_data_create()
{
	return new obs_data;
}
void obs_data_release(obs_data_t *data)
{
	delete data;
}
void obs_data_set_bool(obs_data_t *data, const char *key, bool value)
{
	if (std::string(key) == "restart_on_activate")
		data->restartOnActivate = value;
}
void obs_data_set_string(obs_data_t *data, const char *, const char *value)
{
	data->file = value;
}
obs_source_t *obs_source_create_private(const char *, const char *, obs_data_t *data)
{
	CHECK(data->restartOnActivate); // Selecting an audio file must not auto-play it.
	++created;
	return files[data->file] = new obs_source;
}
void obs_source_release(obs_source_t *source)
{
	CHECK(source->showing == 0 && !source->capture);
	++released;
	for (auto it = files.begin(); it != files.end(); ++it) {
		if (it->second == source) {
			files.erase(it);
			break;
		}
	}
	delete source;
}
void obs_source_set_volume(obs_source_t *source, float volume)
{
	source->volume = volume;
}
static void emitAudio(obs_source_t *source, bool muted = false)
{
	float samples[8]{0.25f};
	audio_data block{};
	block.frames = 8;
	block.timestamp = 123456;
	block.data[0] = reinterpret_cast<uint8_t *>(samples);
	if (source->capture)
		source->capture(source->param, source, &block, muted);
}
void obs_source_media_restart(obs_source_t *source)
{
	// Matches FFmpeg's production precondition which caused the silent mixer regression.
	if (source->showing > 0)
		emitAudio(source);
}
void obs_source_media_stop(obs_source_t *) {}
void obs_source_media_play_pause(obs_source_t *source, bool paused)
{
	source->paused = paused;
}
bool obs_source_active(const obs_source_t *source)
{
	return source->active;
}
void obs_source_inc_showing(obs_source_t *source)
{
	CHECK(++source->showing == 1);
}
void obs_source_dec_showing(obs_source_t *source)
{
	CHECK(--source->showing == 0);
}
void obs_source_add_audio_capture_callback(obs_source_t *s, obs_source_audio_capture_t cb, void *p)
{
	s->capture = cb;
	s->param = p;
}
void obs_source_remove_audio_capture_callback(obs_source_t *s, obs_source_audio_capture_t, void *)
{
	s->capture = nullptr;
}
bool obs_get_audio_info(obs_audio_info *info)
{
	*info = {};
	return true;
}
void obs_source_output_audio(obs_source_t *source, const obs_source_audio *block)
{
	CHECK(block->format == AUDIO_FORMAT_FLOAT_PLANAR && block->samples_per_sec == 48000);
	CHECK(block->timestamp == 123456 && block->data[0]);
	source->frames += block->frames;
	source->sample = reinterpret_cast<const float *>(block->data[0])[0];
}

int main()
{
	obs_source_t parent;
	parent.volume = 0.35f; // User's mixer fader is independent of the plugin gain.
	{
		ChzzkAlertAudio audio(&parent);
		audio.update(true, "one", 70);
		CHECK(files["one"]->showing == 1 && parent.frames == 0);
		emitAudio(files["one"]);
		CHECK(parent.frames == 0); // No unsolicited sound before an alert.
		audio.play();
		CHECK(parent.frames == 8 && parent.volume == 0.35f);
		CHECK(std::abs(parent.sample - 0.25f * 0.7f) < 0.00001f);
		audio.update(true, "one", 50);
		emitAudio(files["one"]);
		CHECK(parent.frames == 16); // Unrelated settings updates must not cut playback.
		CHECK(parent.volume == 0.35f && parent.sample == 0.125f);
		audio.update(true, "two", 100);
		CHECK(files["one"]->showing == 0 && files["two"]->showing == 1);
		emitAudio(files["one"]);
		CHECK(parent.frames == 16);
		audio.play();
		CHECK(parent.frames == 24);
		emitAudio(files["two"], true);
		CHECK(parent.frames == 24);
		audio.update(false, "two", 100);
		audio.play();
		CHECK(files["two"]->showing == 0 && parent.frames == 24);
		audio.update(true, "one", 100);
		audio.play();
		CHECK(parent.frames == 32 && files.size() == 2);
		parent.active = false;
		audio.pause();
		emitAudio(files["one"]);
		audio.play();
		CHECK(parent.frames == 32 && files["one"]->paused);
		parent.active = true;
		audio.resume();
		emitAudio(files["one"]);
		CHECK(parent.frames == 40 && !files["one"]->paused);
		for (int i = 0; i < 12; ++i) {
			audio.update(true, "file" + std::to_string(i), 100);
			CHECK(files.size() <= 4);
			audio.play();
		}
		CHECK(released == created - 4);
		audio.update(true, "one", 100); // Evicted files can be recreated safely.
		audio.play();
		CHECK(files.count("one") == 1 && parent.volume == 0.35f);
		const int beforeHiddenChange = parent.frames;
		audio.pause();
		audio.update(true, "hidden-selection", 100);
		audio.resume(); // Selecting another sound while hidden must not start an unsolicited alert.
		emitAudio(files["hidden-selection"]);
		CHECK(parent.frames == beforeHiddenChange);
		audio.play();
		CHECK(parent.frames == beforeHiddenChange + 8);
	}
	CHECK(released == created && files.empty());
	std::cout << "Audio lifecycle and forwarding checks passed\n";
}
