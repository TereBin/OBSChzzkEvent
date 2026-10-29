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
	gain.store(static_cast<float>(std::clamp(volumePercent, 0, 100)) / 100.0f, std::memory_order_relaxed);
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
		recentFiles.erase(std::remove(recentFiles.begin(), recentFiles.end(), audioFile), recentFiles.end());
		recentFiles.push_back(audioFile);
		trimCache();
	}

	activeSource.store(source, std::memory_order_release);
}

void ChzzkAlertAudio::play()
{
	obs_source_t *active = activeSource.load(std::memory_order_acquire);
	if (active && obs_source_active(outputSource)) {
		pausedPlayback = false;
		playbackReady.store(true, std::memory_order_release);
		obs_source_media_restart(active);
	}
}

void ChzzkAlertAudio::pause()
{
	pausedPlayback = playbackReady.exchange(false, std::memory_order_acq_rel);
	if (source && pausedPlayback)
		obs_source_media_play_pause(source, true);
}

void ChzzkAlertAudio::resume()
{
	if (source && pausedPlayback && obs_source_active(outputSource)) {
		pausedPlayback = false;
		playbackReady.store(true, std::memory_order_release);
		obs_source_media_play_pause(source, false);
	}
}

void ChzzkAlertAudio::captureAudio(void *param, obs_source_t *capturedSource, const audio_data *audioData, bool muted)
{
	static_cast<ChzzkAlertAudio *>(param)->outputAudio(capturedSource, audioData, muted);
}

void ChzzkAlertAudio::outputAudio(obs_source_t *capturedSource, const audio_data *audioData, bool muted)
{
	std::lock_guard lock(captureMutex);
	if (!ready() || capturedSource != activeSource.load(std::memory_order_acquire) || muted || !outputSource ||
	    !obs_source_active(outputSource))
		return;
	obs_audio_info audioInfo = {};
	if (!obs_get_audio_info(&audioInfo))
		return;

	obs_source_audio output = {};
	const float volume = gain.load(std::memory_order_relaxed);
	// Reuse buffers across blocks without touching the OBS mixer gain. Capture
	// callbacks are serialized so late data from another decoder cannot race.
	for (size_t channel = 0; channel < MAX_AV_PLANES; ++channel) {
		if (!audioData->data[channel])
			continue;
		if (volume == 1.0f) {
			output.data[channel] = audioData->data[channel];
			continue;
		}
		const float *samples = reinterpret_cast<const float *>(audioData->data[channel]);
		sampleBuffers[channel].resize(audioData->frames);
		std::transform(samples, samples + audioData->frames, sampleBuffers[channel].begin(),
			       [volume](float sample) { return sample * volume; });
		output.data[channel] = reinterpret_cast<const uint8_t *>(sampleBuffers[channel].data());
	}
	output.frames = audioData->frames;
	output.speakers = audioInfo.speakers;
	output.format = AUDIO_FORMAT_FLOAT_PLANAR;
	output.samples_per_sec = audioInfo.samples_per_sec;
	output.timestamp = audioData->timestamp;
	obs_source_output_audio(outputSource, &output);
}

void ChzzkAlertAudio::trimCache()
{
	constexpr size_t maxCachedSounds = 4;
	while (recentFiles.size() > maxCachedSounds) {
		const auto oldest = recentFiles.front();
		recentFiles.erase(recentFiles.begin());
		auto found = sources.find(oldest);
		if (found == sources.end())
			continue;
		obs_source_remove_audio_capture_callback(found->second.get(), captureAudio, this);
		obs_source_media_stop(found->second.get());
		sources.erase(found);
	}
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
	pausedPlayback = false;
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
	recentFiles.clear();
}

bool ChzzkAlertAudio::ready() const
{
	return activeSource.load(std::memory_order_acquire) && playbackReady.load(std::memory_order_acquire);
}
