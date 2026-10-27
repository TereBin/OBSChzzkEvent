#pragma once

#include <obs.h>
#include "chzzk-obs-resource.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>

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
	void outputAudio(obs_source_t *capturedSource, const audio_data *audioData, bool muted) const;
	bool createSource(const std::string &audioFile);
	void deactivateSource();
	void releaseSources();
	bool ready() const;

	obs_source_t *source = nullptr;
	obs_source_t *outputSource = nullptr;
	std::string currentFile;
	std::unordered_map<std::string, ObsSourcePtr> sources;
	std::atomic<obs_source_t *> activeSource = nullptr;
	std::atomic_bool playbackReady = false;
};
