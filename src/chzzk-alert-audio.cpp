#include "chzzk-alert-audio.hpp"

#include <obs-module.h>

#include <algorithm>

namespace {
constexpr auto kAudioSourceId = "ffmpeg_source";
}

ChzzkAlertAudio::ChzzkAlertAudio(obs_source_t *alertSource) : outputSource(alertSource)
{
	source = obs_source_create_private(kAudioSourceId, "OBS Chzzk Alerts Sound", nullptr);
	if (source) {
		obs_source_add_audio_capture_callback(source, captureAudio, this);
		obs_source_inc_active(source);
		obs_source_inc_showing(source);
	}
}

ChzzkAlertAudio::~ChzzkAlertAudio()
{
	if (source) {
		obs_source_remove_audio_capture_callback(source, captureAudio, this);
		obs_source_dec_showing(source);
		obs_source_dec_active(source);
		obs_source_release(source);
	}
}

void ChzzkAlertAudio::update(bool useSound, const std::string &audioFile, int volumePercent)
{
	enabled = useSound;
	file = audioFile;
	if (!source)
		return;

	obs_data_t *settings = obs_data_create();
	obs_data_set_bool(settings, "is_local_file", true);
	obs_data_set_string(settings, "local_file", file.c_str());
	obs_data_set_bool(settings, "looping", false);
	obs_data_set_bool(settings, "restart_on_activate", false);
	obs_data_set_bool(settings, "close_when_inactive", false);
	obs_source_update(source, settings);
	obs_data_release(settings);

	const int clampedVolume = std::clamp(volumePercent, 0, 100);
	obs_source_set_volume(outputSource, static_cast<float>(clampedVolume) / 100.0f);
	obs_source_media_stop(source);
}

void ChzzkAlertAudio::play()
{
	if (available())
		obs_source_media_restart(source);
}

void ChzzkAlertAudio::captureAudio(void *param, obs_source_t *, const audio_data *audioData, bool muted)
{
	static_cast<ChzzkAlertAudio *>(param)->outputAudio(audioData, muted);
}

void ChzzkAlertAudio::outputAudio(const audio_data *audioData, bool muted) const
{
	if (!available() || muted || !outputSource)
		return;
	obs_audio_info audioInfo = {};
	if (!obs_get_audio_info(&audioInfo))
		return;

	obs_source_audio output = {};
	for (size_t channel = 0; channel < MAX_AV_PLANES; ++channel)
		output.data[channel] = audioData->data[channel];
	output.frames = audioData->frames;
	output.speakers = audioInfo.speakers;
	output.format = AUDIO_FORMAT_FLOAT_PLANAR;
	output.samples_per_sec = audioInfo.samples_per_sec;
	output.timestamp = audioData->timestamp;
	obs_source_output_audio(outputSource, &output);
}

bool ChzzkAlertAudio::available() const
{
	return enabled && source && !file.empty();
}
