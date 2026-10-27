#pragma once

#include <obs.h>

#include <cstddef>
#include <cstdint>
#include <string>

class ChzzkAlertAudio {
public:
	ChzzkAlertAudio();
	~ChzzkAlertAudio();

	ChzzkAlertAudio(const ChzzkAlertAudio &) = delete;
	ChzzkAlertAudio &operator=(const ChzzkAlertAudio &) = delete;

	void update(bool enabled, const std::string &file, int volumePercent);
	void play();
	void enumerate(obs_source_t *parent, obs_source_enum_proc_t callback, void *param) const;
	bool render(uint64_t *timestamp, obs_source_audio_mix *audioOutput, uint32_t mixers, size_t channels) const;

private:
	bool available() const;

	obs_source_t *source = nullptr;
	std::string file;
	bool enabled = false;
};
