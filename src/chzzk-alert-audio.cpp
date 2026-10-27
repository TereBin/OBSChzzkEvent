#include "chzzk-alert-audio.hpp"

#include <obs-module.h>

#include <algorithm>

namespace {
constexpr auto kAudioSourceId = "ffmpeg_source";
}

ChzzkAlertAudio::ChzzkAlertAudio(obs_source_t *alertSource) : outputSource(alertSource) {}

ChzzkAlertAudio::~ChzzkAlertAudio()
{
	releaseSources();
}

void ChzzkAlertAudio::update(bool useSound, const std::string &audioFile, int volumePercent)
{
	if (!useSound || audioFile.empty()) {
		deactivateSource();
		return;
	}
	if (currentFile != audioFile) {
		deactivateSource();
		auto existing = sources.find(audioFile);
		if (existing != sources.end()) {
			source = existing->second.get();
			currentFile = audioFile;
		} else if (!createSource(audioFile)) {
			return;
		}
		// FFmpeg's restart callback only starts playback for a showing source.
		// An auxiliary showing reference enables decoding without routing the
		// private source directly to the main mix (the parent forwards its audio).
		obs_source_inc_showing(source);
	}

	const int clampedVolume = std::clamp(volumePercent, 0, 100);
	obs_source_set_volume(outputSource, static_cast<float>(clampedVolume) / 100.0f);
	activeSource.store(source, std::memory_order_release);
}

void ChzzkAlertAudio::play()
{
	obs_source_t *active = activeSource.load(std::memory_order_acquire);
	if (active) {
		playbackReady.store(true, std::memory_order_release);
		obs_source_media_restart(active);
	}
}

void ChzzkAlertAudio::captureAudio(void *param, obs_source_t *capturedSource, const audio_data *audioData, bool muted)
{
	static_cast<ChzzkAlertAudio *>(param)->outputAudio(capturedSource, audioData, muted);
}

void ChzzkAlertAudio::outputAudio(obs_source_t *capturedSource, const audio_data *audioData, bool muted) const
{
	if (!ready() || capturedSource != activeSource.load(std::memory_order_acquire) || muted || !outputSource)
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

bool ChzzkAlertAudio::createSource(const std::string &audioFile)
{
	auto settings = makeObsData();
	obs_data_set_bool(settings.get(), "is_local_file", true);
	obs_data_set_string(settings.get(), "local_file", audioFile.c_str());
	obs_data_set_bool(settings.get(), "looping", false);
	obs_data_set_bool(settings.get(), "restart_on_activate", true);
	obs_data_set_bool(settings.get(), "close_when_inactive", false);
	ObsSourcePtr created(obs_source_create_private(kAudioSourceId, "OBS Chzzk Alerts Sound", settings.get()));
	source = created.get();
	if (!source)
		return false;
	currentFile = audioFile;
	sources.emplace(audioFile, std::move(created));
	obs_source_add_audio_capture_callback(source, captureAudio, this);
	obs_source_media_stop(source);
	return true;
}

void ChzzkAlertAudio::deactivateSource()
{
	playbackReady.store(false, std::memory_order_release);
	activeSource.store(nullptr, std::memory_order_release);
	if (!source)
		return;
	obs_source_media_stop(source);
	obs_source_dec_showing(source);
	source = nullptr;
	currentFile.clear();
}

void ChzzkAlertAudio::releaseSources()
{
	deactivateSource();
	for (const auto &[file, cachedSource] : sources) {
		(void)file;
		obs_source_remove_audio_capture_callback(cachedSource.get(), captureAudio, this);
		obs_source_media_stop(cachedSource.get());
	}
	sources.clear();
}

bool ChzzkAlertAudio::ready() const
{
	return activeSource.load(std::memory_order_acquire) && playbackReady.load(std::memory_order_acquire);
}
