#include "chzzk-alert-audio.hpp"

#include <obs-module.h>

#include <algorithm>
#include <cstring>

namespace {
constexpr auto kAudioSourceId = "ffmpeg_source";
}

ChzzkAlertAudio::ChzzkAlertAudio()
{
	source = obs_source_create_private(kAudioSourceId, "OBS Chzzk Alerts Sound", nullptr);
}

ChzzkAlertAudio::~ChzzkAlertAudio()
{
	if (source)
		obs_source_release(source);
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
	obs_source_set_volume(source, static_cast<float>(clampedVolume) / 100.0f);
	obs_source_media_stop(source);
}

void ChzzkAlertAudio::play()
{
	if (available())
		obs_source_media_restart(source);
}

void ChzzkAlertAudio::enumerate(obs_source_t *parent, obs_source_enum_proc_t callback, void *param) const
{
	if (available())
		callback(parent, source, param);
}

bool ChzzkAlertAudio::render(uint64_t *timestamp, obs_source_audio_mix *audioOutput, uint32_t mixers,
			     size_t channels) const
{
	if (!available() || obs_source_audio_pending(source))
		return false;
	const uint64_t sourceTimestamp = obs_source_get_audio_timestamp(source);
	if (!sourceTimestamp)
		return false;

	obs_source_audio_mix childAudio = {};
	obs_source_get_audio_mix(source, &childAudio);
	for (size_t mix = 0; mix < MAX_AUDIO_MIXES; ++mix) {
		if ((mixers & (1u << mix)) == 0)
			continue;
		for (size_t channel = 0; channel < channels; ++channel)
			std::memcpy(audioOutput->output[mix].data[channel], childAudio.output[mix].data[channel],
				    AUDIO_OUTPUT_FRAMES * sizeof(float));
	}
	*timestamp = sourceTimestamp;
	return true;
}

bool ChzzkAlertAudio::available() const
{
	return enabled && source && !file.empty();
}
