#pragma once
#include <cstdint>
#define MAX_AV_PLANES 8
enum audio_format { AUDIO_FORMAT_FLOAT_PLANAR };
struct obs_source;
struct obs_data;
using obs_source_t = obs_source;
using obs_data_t = obs_data;
struct audio_data {
	uint8_t *data[MAX_AV_PLANES]{};
	uint32_t frames = 0;
	uint64_t timestamp = 0;
};
struct obs_audio_info {
	uint32_t samples_per_sec = 48000;
	int speakers = 2;
};
struct obs_source_audio {
	const uint8_t *data[MAX_AV_PLANES]{};
	uint32_t frames = 0;
	int speakers = 0;
	audio_format format = AUDIO_FORMAT_FLOAT_PLANAR;
	uint32_t samples_per_sec = 0;
	uint64_t timestamp = 0;
};
using obs_source_audio_capture_t = void (*)(void *, obs_source_t *, const audio_data *, bool);
obs_data_t *obs_data_create();
void obs_data_release(obs_data_t *);
void obs_data_set_bool(obs_data_t *, const char *, bool);
void obs_data_set_string(obs_data_t *, const char *, const char *);
obs_source_t *obs_source_create_private(const char *, const char *, obs_data_t *);
void obs_source_release(obs_source_t *);
void obs_source_set_volume(obs_source_t *, float);
void obs_source_media_restart(obs_source_t *);
void obs_source_media_stop(obs_source_t *);
void obs_source_inc_showing(obs_source_t *);
void obs_source_dec_showing(obs_source_t *);
void obs_source_add_audio_capture_callback(obs_source_t *, obs_source_audio_capture_t, void *);
void obs_source_remove_audio_capture_callback(obs_source_t *, obs_source_audio_capture_t, void *);
bool obs_get_audio_info(obs_audio_info *);
void obs_source_output_audio(obs_source_t *, const obs_source_audio *);
