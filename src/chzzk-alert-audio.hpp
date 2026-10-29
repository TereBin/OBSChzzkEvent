#pragma once

#include <obs.h>
#include "chzzk-obs-resource.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

class ChzzkAlertAudio {
public:
	explicit ChzzkAlertAudio(obs_source_t *outputSource);
	~ChzzkAlertAudio();

	ChzzkAlertAudio(const ChzzkAlertAudio &) = delete;
	ChzzkAlertAudio &operator=(const ChzzkAlertAudio &) = delete;

	void update(bool enabled, const std::string &file, int volumePercent);
	void play();
	void pause();
	void resume();

private:
	static void captureAudio(void *param, obs_source_t *source, const audio_data *audioData, bool muted);
	void outputAudio(obs_source_t *capturedSource, const audio_data *audioData, bool muted);
	bool createSource(const std::string &audioFile);
	void deactivateSource();
	void releaseSources();
	void trimCache();
	bool ready() const;

	obs_source_t *source = nullptr;
	obs_source_t *outputSource = nullptr;
	std::string currentFile;
	std::unordered_map<std::string, ObsSourcePtr> sources;
	std::vector<std::string> recentFiles;
	std::atomic<float> gain{1.0f};
	std::mutex captureMutex;
	std::vector<float> sampleBuffers[MAX_AV_PLANES];
	std::atomic<obs_source_t *> activeSource = nullptr;
	std::atomic_bool playbackReady = false;
	bool pausedPlayback = false;
};
