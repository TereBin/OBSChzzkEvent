#pragma once

#include <obs.h>

#include <cstddef>
#include <cstdint>
#include <string>

class ChzzkAlertAudio {
public:
	explicit ChzzkAlertAudio(obs_source_t *outputSource);
	~ChzzkAlertAudio();

	ChzzkAlertAudio(const ChzzkAlertAudio &) = delete;
	ChzzkAlertAudio &operator=(const ChzzkAlertAudio &) = delete;

	void update(bool enabled, const std::string &file, int volumePercent);
	void play();

private:
	static void captureAudio(void *param, obs_source_t *source, const audio_data *audioData, bool muted);
	void outputAudio(const audio_data *audioData, bool muted) const;
	bool available() const;

	obs_source_t *source = nullptr;
	obs_source_t *outputSource = nullptr;
	std::string file;
	bool enabled = false;
};
