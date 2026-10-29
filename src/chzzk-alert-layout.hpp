#pragma once

#include <algorithm>
#include <cstdint>
#include <string_view>

struct AlertLayout {
	float textX = 0.0f;
	float textY = 0.0f;
	uint32_t textWidth;
	uint32_t textHeight;
	float mediaX = 0.0f;
	float mediaY = 0.0f;
	uint32_t mediaWidth;
	uint32_t mediaHeight;
};

// Inputs come from the clamped source settings; zero media size means text only.
inline AlertLayout calculateAlertLayout(uint32_t width, uint32_t height, uint32_t mediaWidth, uint32_t mediaHeight,
					uint32_t margin, std::string_view position)
{
	AlertLayout result{0, 0, width, height, 0, 0, std::min(mediaWidth, width), std::min(mediaHeight, height)};
	if (mediaWidth == 0 || mediaHeight == 0)
		return result;
	if (position == "left" || position == "right") {
		const uint32_t reserved = std::min(width - 1, result.mediaWidth + margin);
		result.textWidth = width - reserved;
		result.mediaY = static_cast<float>(height - result.mediaHeight) / 2.0f;
		if (position == "left")
			result.textX = static_cast<float>(reserved);
		else
			result.mediaX = static_cast<float>(width - result.mediaWidth);
	} else if (position == "top" || position == "bottom") {
		const uint32_t reserved = std::min(height - 1, result.mediaHeight + margin);
		result.textHeight = height - reserved;
		result.mediaX = static_cast<float>(width - result.mediaWidth) / 2.0f;
		if (position == "top")
			result.textY = static_cast<float>(reserved);
		else
			result.mediaY = static_cast<float>(height - result.mediaHeight);
	} else {
		result.mediaWidth = width;
		result.mediaHeight = height;
	}
	return result;
}
