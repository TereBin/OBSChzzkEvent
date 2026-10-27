#pragma once

#include "chzzk-obs-resource.hpp"

#include <obs-properties.h>
#include <cstddef>
#include <cstdint>
#include <string>

// Persisted keys and UI labels live in the implementation; runtime values live here.
struct AlertSettings {
	std::string messageTemplate;
	std::string alignment;
	std::string mediaFile;
	std::string mediaPosition;
	uint32_t textColor = 0xFFFFFF;
	uint32_t backgroundColor = 0;
	uint32_t outlineColor = 0;
	uint32_t shadowColor = 0;
	uint32_t width = 800;
	uint32_t height = 200;
	uint32_t mediaWidth = 160;
	uint32_t mediaHeight = 160;
	uint32_t mediaMargin = 20;
	int backgroundOpacity = 55;
	int outlineSize = 3;
	int shadowOpacityPercent = 70;
	int shadowDistance = 4;
	int dedupeSeconds = 30;
	int mediaOpacityPercent = 100;
	size_t maxQueue = 20;
	float displaySeconds = 5.0f;
	float fadeInSeconds = 0.3f;
	float fadeOutSeconds = 0.3f;
	bool outline = true;
	bool shadow = true;
	bool mediaEnabled = false;
	bool mediaKeepAspect = true;
	bool mediaRestart = true;
	ObsDataPtr font;
	bool soundEnabled = false;
	std::string soundFile;
	int soundVolume = 100;
};

AlertSettings readAlertSettings(obs_data_t *settings);
void setAlertDefaults(obs_data_t *settings);
obs_properties_t *createAlertProperties(void *data, obs_property_clicked_t testButtonClicked);
