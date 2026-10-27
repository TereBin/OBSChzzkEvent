#include "chzzk-alert-source.hpp"
#include "chzzk-alert-audio.hpp"
#include "chzzk-alert-settings.hpp"
#include "chzzk-alert-model.hpp"
#include "chzzk-alert-visual.hpp"

#include <obs-module.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace {
constexpr auto kSourceId = "obs_chzzk_follow_alert";
constexpr auto kTextSourceId = "text_gdiplus";
constexpr auto kImageSourceId = "image_source";

struct AlertSource;
std::mutex g_sourcesMutex;
std::vector<AlertSource *> g_sources;

struct AlertSource {
	explicit AlertSource(obs_data_t *settings, obs_source_t *source) : alertAudio(source)
	{
		update(settings);
		setOpacity(0.0f, true);
		std::lock_guard lock(g_sourcesMutex);
		g_sources.push_back(this);
	}

	~AlertSource()
	{
		{
			std::lock_guard lock(g_sourcesMutex);
			g_sources.erase(std::remove(g_sources.begin(), g_sources.end(), this), g_sources.end());
		}
	}

	void update(obs_data_t *settings)
	{
		std::lock_guard lock(mutex);
		config = readAlertSettings(settings);
		const uint32_t shadowPadding = config.shadow ? static_cast<uint32_t>(config.shadowDistance) : 0;
		// OBS/Stream Deck can query dimensions synchronously while this mutex is held.
		outputSize.update(config.width, config.height, shadowPadding);
		queue.configure(config.maxQueue, config.dedupeSeconds);
		alertAudio.update(config.soundEnabled, config.soundFile, config.soundVolume);

		updateMediaChild();
		if (active)
			updateTextChildren(currentNickname);
		if (opacityStep >= 0)
			setOpacity(static_cast<float>(opacityStep) / 20.0f, true);
	}

	void enqueue(const std::string &nickname, bool isTest = false)
	{
		std::lock_guard lock(mutex);
		queue.enqueue(nickname, isTest);
	}

	void tick(float seconds)
	{
		std::lock_guard lock(mutex);
		if (!active) {
			if (queue.empty())
				return;
			startNextAlert();
		}

		elapsed += seconds;
		const AlertTiming timing{config.fadeInSeconds, config.displaySeconds, config.fadeOutSeconds};
		if (timing.finished(elapsed)) {
			active = false;
			currentNickname.clear();
			setOpacity(0.0f);
			return;
		}
		setOpacity(timing.opacity(elapsed));
	}

	void render()
	{
		std::lock_guard lock(mutex);
		if (!active || opacityStep <= 0)
			return;
		const Layout layout = calculateLayout();
		renderMedia(layout);
		if (config.shadow && shadowText) {
			gs_matrix_push();
			gs_matrix_translate3f(layout.textX + static_cast<float>(config.shadowDistance),
					      layout.textY + static_cast<float>(config.shadowDistance), 0.0f);
			obs_source_video_render(shadowText.get());
			gs_matrix_pop();
		}
		if (text) {
			gs_matrix_push();
			gs_matrix_translate3f(layout.textX, layout.textY, 0.0f);
			obs_source_video_render(text.get());
			gs_matrix_pop();
		}
	}

	uint32_t outputWidth() const { return outputSize.width(); }
	uint32_t outputHeight() const { return outputSize.height(); }

private:
	struct Layout {
		float textX = 0.0f;
		float textY = 0.0f;
		uint32_t textWidth = 1;
		uint32_t textHeight = 1;
		float mediaX = 0.0f;
		float mediaY = 0.0f;
		uint32_t mediaWidth = 0;
		uint32_t mediaHeight = 0;
	};

	bool hasMedia() const { return config.mediaEnabled && media && !config.mediaFile.empty(); }

	Layout calculateLayout() const
	{
		Layout layout;
		layout.textWidth = config.width;
		layout.textHeight = config.height;
		if (!hasMedia())
			return layout;

		layout.mediaWidth = std::min(config.mediaWidth, config.width);
		layout.mediaHeight = std::min(config.mediaHeight, config.height);
		if (config.mediaPosition == "left" || config.mediaPosition == "right") {
			const uint32_t reserved = std::min(config.width - 1, layout.mediaWidth + config.mediaMargin);
			layout.textWidth = config.width - reserved;
			layout.mediaY = static_cast<float>(config.height - layout.mediaHeight) / 2.0f;
			if (config.mediaPosition == "left")
				layout.textX = static_cast<float>(reserved);
			else
				layout.mediaX = static_cast<float>(config.width - layout.mediaWidth);
		} else if (config.mediaPosition == "top" || config.mediaPosition == "bottom") {
			const uint32_t reserved = std::min(config.height - 1, layout.mediaHeight + config.mediaMargin);
			layout.textHeight = config.height - reserved;
			layout.mediaX = static_cast<float>(config.width - layout.mediaWidth) / 2.0f;
			if (config.mediaPosition == "top")
				layout.textY = static_cast<float>(reserved);
			else
				layout.mediaY = static_cast<float>(config.height - layout.mediaHeight);
		} else {
			layout.mediaWidth = config.width;
			layout.mediaHeight = config.height;
		}
		return layout;
	}

	void startNextAlert()
	{
		currentNickname = queue.pop();
		elapsed = 0.0f;
		active = true;
		updateTextChildren(currentNickname);
		if (hasMedia() && config.mediaRestart)
			obs_source_media_restart(media.get());
		alertAudio.play();
	}

	void updateTextChildren(const std::string &nickname)
	{
		const Layout layout = calculateLayout();
		const std::string message = formatAlertMessage(config.messageTemplate, nickname);
		updateTextChild(text.get(), message, config.textColor, config.backgroundColor, config.backgroundOpacity,
				config.outline, config.outlineSize, config.outlineColor, layout.textWidth,
				layout.textHeight);
		updateTextChild(shadowText.get(), message, config.shadowColor, 0, 0, false, 1, 0, layout.textWidth,
				layout.textHeight);
	}

	void updateTextChild(obs_source_t *child, const std::string &message, uint32_t color, uint32_t bkColor,
			     int bkOpacity, bool useOutline, int useOutlineSize, uint32_t useOutlineColor,
			     uint32_t textWidth, uint32_t textHeight)
	{
		if (!child)
			return;
		auto settings = makeObsData();
		obs_data_set_string(settings.get(), "text", message.c_str());
		if (config.font)
			obs_data_set_obj(settings.get(), "font", config.font.get());
		obs_data_set_int(settings.get(), "color", color);
		obs_data_set_int(settings.get(), "opacity", 100);
		obs_data_set_int(settings.get(), "bk_color", bkColor);
		obs_data_set_int(settings.get(), "bk_opacity", bkOpacity);
		obs_data_set_string(settings.get(), "align", config.alignment.c_str());
		obs_data_set_string(settings.get(), "valign", "center");
		obs_data_set_bool(settings.get(), "outline", useOutline);
		obs_data_set_int(settings.get(), "outline_size", useOutlineSize);
		obs_data_set_int(settings.get(), "outline_color", useOutlineColor);
		obs_data_set_int(settings.get(), "outline_opacity", 100);
		obs_data_set_bool(settings.get(), "extents", true);
		obs_data_set_int(settings.get(), "extents_cx", textWidth);
		obs_data_set_int(settings.get(), "extents_cy", textHeight);
		obs_data_set_bool(settings.get(), "extents_wrap", true);
		obs_source_update(child, settings.get());
	}

	void updateMediaChild()
	{
		if (!media)
			return;
		auto settings = makeObsData();
		obs_data_set_string(settings.get(), "file", config.mediaFile.c_str());
		obs_data_set_bool(settings.get(), "unload", false);
		obs_source_update(media.get(), settings.get());
	}

	void renderMedia(const Layout &layout)
	{
		if (!hasMedia() || layout.mediaWidth == 0 || layout.mediaHeight == 0)
			return;
		const uint32_t sourceWidth = obs_source_get_width(media.get());
		const uint32_t sourceHeight = obs_source_get_height(media.get());
		if (sourceWidth == 0 || sourceHeight == 0)
			return;

		float scaleX = static_cast<float>(layout.mediaWidth) / static_cast<float>(sourceWidth);
		float scaleY = static_cast<float>(layout.mediaHeight) / static_cast<float>(sourceHeight);
		float x = layout.mediaX;
		float y = layout.mediaY;
		if (config.mediaKeepAspect) {
			const float scale = std::min(scaleX, scaleY);
			scaleX = scale;
			scaleY = scale;
			x += (static_cast<float>(layout.mediaWidth) - static_cast<float>(sourceWidth) * scale) / 2.0f;
			y += (static_cast<float>(layout.mediaHeight) - static_cast<float>(sourceHeight) * scale) / 2.0f;
		}

		gs_matrix_push();
		gs_matrix_translate3f(x, y, 0.0f);
		gs_matrix_scale3f(scaleX, scaleY, 1.0f);
		obs_source_video_render(media.get());
		gs_matrix_pop();
	}

	void setOpacity(float alpha, bool force = false)
	{
		const int step = std::clamp(static_cast<int>(std::lround(alpha * 20.0f)), 0, 20);
		if (!force && step == opacityStep)
			return;
		opacityStep = step;
		const double opacity = static_cast<double>(step) * 5.0;
		text.setOpacity(opacity);
		shadowText.setOpacity(opacity * static_cast<double>(config.shadowOpacityPercent) / 100.0);
		media.setOpacity(opacity * static_cast<double>(config.mediaOpacityPercent) / 100.0);
	}

	// State must outlive child cleanup: OBS signals can re-enter size queries during release.
	AlertSettings config;
	std::mutex mutex;
	AlertSizeSnapshot outputSize;
	AlertQueue queue;
	std::string currentNickname;
	int opacityStep = -1;
	float elapsed = 0.0f;
	bool active = false;
	AlertVisual text{kTextSourceId, "OBS Chzzk Alerts Text", "OBS Chzzk Alerts Text Opacity"};
	AlertVisual shadowText{kTextSourceId, "OBS Chzzk Alerts Shadow", "OBS Chzzk Alerts Shadow Opacity"};
	AlertVisual media{kImageSourceId, "OBS Chzzk Alerts Media", "OBS Chzzk Alerts Media Opacity"};
	ChzzkAlertAudio alertAudio;
};

const char *sourceName(void *)
{
	return obs_module_text("Source.Name");
}

void *sourceCreate(obs_data_t *settings, obs_source_t *source)
{
	return new AlertSource(settings, source);
}

void sourceDestroy(void *data)
{
	delete static_cast<AlertSource *>(data);
}

void sourceUpdate(void *data, obs_data_t *settings)
{
	static_cast<AlertSource *>(data)->update(settings);
}

void sourceTick(void *data, float seconds)
{
	static_cast<AlertSource *>(data)->tick(seconds);
}

void sourceRender(void *data, gs_effect_t *)
{
	static_cast<AlertSource *>(data)->render();
}

uint32_t sourceWidth(void *data)
{
	return static_cast<AlertSource *>(data)->outputWidth();
}

uint32_t sourceHeight(void *data)
{
	return static_cast<AlertSource *>(data)->outputHeight();
}

bool testButtonClicked(obs_properties_t *, obs_property_t *, void *data)
{
	if (data)
		static_cast<AlertSource *>(data)->enqueue(obs_module_text("Source.TestNickname"), true);
	return false;
}

obs_properties_t *sourceProperties(void *data)
{
	return createAlertProperties(data, testButtonClicked);
}

} // namespace

void registerChzzkAlertSource()
{
	obs_source_info info = {};
	info.id = kSourceId;
	info.type = OBS_SOURCE_TYPE_INPUT;
	info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_AUDIO | OBS_SOURCE_CUSTOM_DRAW | OBS_SOURCE_SRGB;
	info.icon_type = OBS_ICON_TYPE_TEXT;
	info.get_name = sourceName;
	info.create = sourceCreate;
	info.destroy = sourceDestroy;
	info.update = sourceUpdate;
	info.get_defaults = setAlertDefaults;
	info.get_properties = sourceProperties;
	info.video_tick = sourceTick;
	info.video_render = sourceRender;
	info.get_width = sourceWidth;
	info.get_height = sourceHeight;
	obs_register_source(&info);
}

void publishFollowToAlertSources(const QString &nickname)
{
	const std::string utf8 = nickname.toUtf8().constData();
	std::lock_guard lock(g_sourcesMutex);
	for (AlertSource *source : g_sources)
		source->enqueue(utf8);
}
