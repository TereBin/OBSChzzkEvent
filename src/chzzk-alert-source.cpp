#include "chzzk-alert-source.hpp"

#include <obs-module.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
constexpr auto kSourceId = "obs_chzzk_follow_alert";
constexpr auto kTextSourceId = "text_gdiplus";
constexpr auto kImageSourceId = "image_source";
constexpr auto kOpacityFilterId = "color_filter";

constexpr auto kTemplate = "message_template";
constexpr auto kFont = "font";
constexpr auto kTextColor = "text_color";
constexpr auto kBackgroundColor = "background_color";
constexpr auto kBackgroundOpacity = "background_opacity";
constexpr auto kOutline = "outline";
constexpr auto kOutlineSize = "outline_size";
constexpr auto kOutlineColor = "outline_color";
constexpr auto kShadow = "shadow";
constexpr auto kShadowColor = "shadow_color";
constexpr auto kShadowOpacity = "shadow_opacity";
constexpr auto kShadowDistance = "shadow_distance";
constexpr auto kAlignment = "alignment";
constexpr auto kWidth = "width";
constexpr auto kHeight = "height";
constexpr auto kDisplaySeconds = "display_seconds";
constexpr auto kFadeInMs = "fade_in_ms";
constexpr auto kFadeOutMs = "fade_out_ms";
constexpr auto kMaxQueue = "max_queue";
constexpr auto kDedupeSeconds = "dedupe_seconds";
constexpr auto kMediaEnabled = "media_enabled";
constexpr auto kMediaFile = "media_file";
constexpr auto kMediaPosition = "media_position";
constexpr auto kMediaWidth = "media_width";
constexpr auto kMediaHeight = "media_height";
constexpr auto kMediaMargin = "media_margin";
constexpr auto kMediaOpacity = "media_opacity";
constexpr auto kMediaKeepAspect = "media_keep_aspect";
constexpr auto kMediaRestart = "media_restart";

struct AlertSource;
std::mutex g_sourcesMutex;
std::vector<AlertSource *> g_sources;

std::string replaceNickname(const std::string &format, const std::string &nickname)
{
	std::string result = format;
	size_t position = 0;
	while ((position = result.find("{nickname}", position)) != std::string::npos) {
		result.replace(position, 10, nickname);
		position += nickname.size();
	}
	return result;
}

void releaseChild(obs_source_t *&child, obs_source_t *&filter)
{
	if (child && filter)
		obs_source_filter_remove(child, filter);
	if (filter)
		obs_source_release(filter);
	if (child)
		obs_source_release(child);
	filter = nullptr;
	child = nullptr;
}

struct AlertSource {
	explicit AlertSource(obs_data_t *settings)
	{
		text = obs_source_create_private(kTextSourceId, "OBS Chzzk Alerts Text", nullptr);
		shadowText = obs_source_create_private(kTextSourceId, "OBS Chzzk Alerts Shadow", nullptr);
		media = obs_source_create_private(kImageSourceId, "OBS Chzzk Alerts Media", nullptr);
		if (text) {
			textOpacity = obs_source_create_private(kOpacityFilterId, "OBS Chzzk Alerts Text Opacity", nullptr);
			if (textOpacity)
				obs_source_filter_add(text, textOpacity);
		}
		if (shadowText) {
			shadowOpacity =
				obs_source_create_private(kOpacityFilterId, "OBS Chzzk Alerts Shadow Opacity", nullptr);
			if (shadowOpacity)
				obs_source_filter_add(shadowText, shadowOpacity);
		}
		if (media) {
			mediaOpacity = obs_source_create_private(kOpacityFilterId, "OBS Chzzk Alerts Media Opacity", nullptr);
			if (mediaOpacity)
				obs_source_filter_add(media, mediaOpacity);
		}

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
		releaseChild(text, textOpacity);
		releaseChild(shadowText, shadowOpacity);
		releaseChild(media, mediaOpacity);
		if (font)
			obs_data_release(font);
	}

	void update(obs_data_t *settings)
	{
		std::lock_guard lock(mutex);
		messageTemplate = obs_data_get_string(settings, kTemplate);
		textColor = static_cast<uint32_t>(obs_data_get_int(settings, kTextColor));
		backgroundColor = static_cast<uint32_t>(obs_data_get_int(settings, kBackgroundColor));
		backgroundOpacity = static_cast<int>(obs_data_get_int(settings, kBackgroundOpacity));
		outline = obs_data_get_bool(settings, kOutline);
		outlineSize = static_cast<int>(obs_data_get_int(settings, kOutlineSize));
		outlineColor = static_cast<uint32_t>(obs_data_get_int(settings, kOutlineColor));
		shadow = obs_data_get_bool(settings, kShadow);
		shadowColor = static_cast<uint32_t>(obs_data_get_int(settings, kShadowColor));
		shadowOpacityPercent = static_cast<int>(obs_data_get_int(settings, kShadowOpacity));
		shadowDistance = static_cast<int>(obs_data_get_int(settings, kShadowDistance));
		alignment = obs_data_get_string(settings, kAlignment);
		width = static_cast<uint32_t>(obs_data_get_int(settings, kWidth));
		height = static_cast<uint32_t>(obs_data_get_int(settings, kHeight));
		displaySeconds = static_cast<float>(obs_data_get_double(settings, kDisplaySeconds));
		fadeInSeconds = static_cast<float>(obs_data_get_int(settings, kFadeInMs)) / 1000.0f;
		fadeOutSeconds = static_cast<float>(obs_data_get_int(settings, kFadeOutMs)) / 1000.0f;
		maxQueue = static_cast<size_t>(std::max<int64_t>(1, obs_data_get_int(settings, kMaxQueue)));
		dedupeSeconds = static_cast<int>(obs_data_get_int(settings, kDedupeSeconds));
		mediaEnabled = obs_data_get_bool(settings, kMediaEnabled);
		mediaFile = obs_data_get_string(settings, kMediaFile);
		mediaPosition = obs_data_get_string(settings, kMediaPosition);
		mediaWidth = static_cast<uint32_t>(obs_data_get_int(settings, kMediaWidth));
		mediaHeight = static_cast<uint32_t>(obs_data_get_int(settings, kMediaHeight));
		mediaMargin = static_cast<uint32_t>(obs_data_get_int(settings, kMediaMargin));
		mediaOpacityPercent = static_cast<int>(obs_data_get_int(settings, kMediaOpacity));
		mediaKeepAspect = obs_data_get_bool(settings, kMediaKeepAspect);
		mediaRestart = obs_data_get_bool(settings, kMediaRestart);

		obs_data_t *fontValue = obs_data_get_obj(settings, kFont);
		if (fontValue) {
			if (font)
				obs_data_release(font);
			font = fontValue;
		}

		updateMediaChild();
		if (active)
			updateTextChildren(currentNickname);
		if (opacityStep >= 0)
			setOpacity(static_cast<float>(opacityStep) / 20.0f, true);
	}

	void enqueue(const std::string &nickname, bool isTest = false)
	{
		if (nickname.empty())
			return;

		std::lock_guard lock(mutex);
		const auto now = std::chrono::steady_clock::now();
		if (!isTest && dedupeSeconds > 0) {
			const auto found = recentlySeen.find(nickname);
			if (found != recentlySeen.end() &&
			    now - found->second < std::chrono::seconds(dedupeSeconds))
				return;
			recentlySeen[nickname] = now;
		}

		pruneRecentlySeen(now);

		if (queue.size() >= maxQueue)
			queue.pop_front();
		queue.push_back(nickname);
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
		const float fadeOutStart = fadeInSeconds + displaySeconds;
		const float total = fadeOutStart + fadeOutSeconds;
		float alpha = 1.0f;
		if (fadeInSeconds > 0.0f && elapsed < fadeInSeconds)
			alpha = elapsed / fadeInSeconds;
		else if (fadeOutSeconds > 0.0f && elapsed > fadeOutStart)
			alpha = (total - elapsed) / fadeOutSeconds;

		if (elapsed >= total) {
			active = false;
			currentNickname.clear();
			setOpacity(0.0f);
			return;
		}
		setOpacity(std::clamp(alpha, 0.0f, 1.0f));
	}

	void render()
	{
		std::lock_guard lock(mutex);
		if (!active || opacityStep <= 0)
			return;
		const Layout layout = calculateLayout();
		renderMedia(layout);
		if (shadow && shadowText) {
			gs_matrix_push();
			gs_matrix_translate3f(layout.textX + static_cast<float>(shadowDistance),
					      layout.textY + static_cast<float>(shadowDistance), 0.0f);
			obs_source_video_render(shadowText);
			gs_matrix_pop();
		}
		if (text) {
			gs_matrix_push();
			gs_matrix_translate3f(layout.textX, layout.textY, 0.0f);
			obs_source_video_render(text);
			gs_matrix_pop();
		}
	}

	uint32_t outputWidth()
	{
		std::lock_guard lock(mutex);
		return width + (shadow ? static_cast<uint32_t>(shadowDistance) : 0);
	}
	uint32_t outputHeight()
	{
		std::lock_guard lock(mutex);
		return height + (shadow ? static_cast<uint32_t>(shadowDistance) : 0);
	}

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

	bool hasMedia() const { return mediaEnabled && media && !mediaFile.empty(); }

	Layout calculateLayout() const
	{
		Layout layout;
		layout.textWidth = width;
		layout.textHeight = height;
		if (!hasMedia())
			return layout;

		layout.mediaWidth = std::min(mediaWidth, width);
		layout.mediaHeight = std::min(mediaHeight, height);
		if (mediaPosition == "left" || mediaPosition == "right") {
			const uint32_t reserved = std::min(width - 1, layout.mediaWidth + mediaMargin);
			layout.textWidth = width - reserved;
			layout.mediaY = static_cast<float>(height - layout.mediaHeight) / 2.0f;
			if (mediaPosition == "left")
				layout.textX = static_cast<float>(reserved);
			else
				layout.mediaX = static_cast<float>(width - layout.mediaWidth);
		} else if (mediaPosition == "top" || mediaPosition == "bottom") {
			const uint32_t reserved = std::min(height - 1, layout.mediaHeight + mediaMargin);
			layout.textHeight = height - reserved;
			layout.mediaX = static_cast<float>(width - layout.mediaWidth) / 2.0f;
			if (mediaPosition == "top")
				layout.textY = static_cast<float>(reserved);
			else
				layout.mediaY = static_cast<float>(height - layout.mediaHeight);
		} else {
			layout.mediaWidth = width;
			layout.mediaHeight = height;
		}
		return layout;
	}

	void startNextAlert()
	{
		currentNickname = std::move(queue.front());
		queue.pop_front();
		elapsed = 0.0f;
		active = true;
		updateTextChildren(currentNickname);
		if (hasMedia() && mediaRestart)
			obs_source_media_restart(media);
	}

	void pruneRecentlySeen(std::chrono::steady_clock::time_point now)
	{
		const auto retention = std::chrono::seconds(std::max(dedupeSeconds, 300));
		for (auto it = recentlySeen.begin(); it != recentlySeen.end();) {
			if (now - it->second > retention)
				it = recentlySeen.erase(it);
			else
				++it;
		}
	}

	void updateTextChildren(const std::string &nickname)
	{
		const Layout layout = calculateLayout();
		const std::string message = replaceNickname(messageTemplate, nickname);
		updateTextChild(text, message, textColor, backgroundColor, backgroundOpacity, outline, outlineSize,
				outlineColor, layout.textWidth, layout.textHeight);
		updateTextChild(shadowText, message, shadowColor, 0, 0, false, 1, 0, layout.textWidth,
				layout.textHeight);
	}

	void updateTextChild(obs_source_t *child, const std::string &message, uint32_t color, uint32_t bkColor,
			     int bkOpacity, bool useOutline, int useOutlineSize, uint32_t useOutlineColor,
			     uint32_t textWidth, uint32_t textHeight)
	{
		if (!child)
			return;
		obs_data_t *settings = obs_data_create();
		obs_data_set_string(settings, "text", message.c_str());
		if (font)
			obs_data_set_obj(settings, "font", font);
		obs_data_set_int(settings, "color", color);
		obs_data_set_int(settings, "opacity", 100);
		obs_data_set_int(settings, "bk_color", bkColor);
		obs_data_set_int(settings, "bk_opacity", bkOpacity);
		obs_data_set_string(settings, "align", alignment.c_str());
		obs_data_set_string(settings, "valign", "center");
		obs_data_set_bool(settings, "outline", useOutline);
		obs_data_set_int(settings, "outline_size", useOutlineSize);
		obs_data_set_int(settings, "outline_color", useOutlineColor);
		obs_data_set_int(settings, "outline_opacity", 100);
		obs_data_set_bool(settings, "extents", true);
		obs_data_set_int(settings, "extents_cx", textWidth);
		obs_data_set_int(settings, "extents_cy", textHeight);
		obs_data_set_bool(settings, "extents_wrap", true);
		obs_source_update(child, settings);
		obs_data_release(settings);
	}

	void updateMediaChild()
	{
		if (!media)
			return;
		obs_data_t *settings = obs_data_create();
		obs_data_set_string(settings, "file", mediaFile.c_str());
		obs_data_set_bool(settings, "unload", false);
		obs_source_update(media, settings);
		obs_data_release(settings);
	}

	void renderMedia(const Layout &layout)
	{
		if (!hasMedia() || layout.mediaWidth == 0 || layout.mediaHeight == 0)
			return;
		const uint32_t sourceWidth = obs_source_get_width(media);
		const uint32_t sourceHeight = obs_source_get_height(media);
		if (sourceWidth == 0 || sourceHeight == 0)
			return;

		float scaleX = static_cast<float>(layout.mediaWidth) / static_cast<float>(sourceWidth);
		float scaleY = static_cast<float>(layout.mediaHeight) / static_cast<float>(sourceHeight);
		float x = layout.mediaX;
		float y = layout.mediaY;
		if (mediaKeepAspect) {
			const float scale = std::min(scaleX, scaleY);
			scaleX = scale;
			scaleY = scale;
			x += (static_cast<float>(layout.mediaWidth) - static_cast<float>(sourceWidth) * scale) / 2.0f;
			y += (static_cast<float>(layout.mediaHeight) - static_cast<float>(sourceHeight) * scale) / 2.0f;
		}

		gs_matrix_push();
		gs_matrix_translate3f(x, y, 0.0f);
		gs_matrix_scale3f(scaleX, scaleY, 1.0f);
		obs_source_video_render(media);
		gs_matrix_pop();
	}

	void setOpacity(float alpha, bool force = false)
	{
		const int step = std::clamp(static_cast<int>(std::lround(alpha * 20.0f)), 0, 20);
		if (!force && step == opacityStep)
			return;
		opacityStep = step;
		const double opacity = static_cast<double>(step) * 5.0;
		updateOpacityFilter(textOpacity, opacity);
		updateOpacityFilter(shadowOpacity, opacity * static_cast<double>(shadowOpacityPercent) / 100.0);
		updateOpacityFilter(mediaOpacity, opacity * static_cast<double>(mediaOpacityPercent) / 100.0);
	}

	static void updateOpacityFilter(obs_source_t *filter, double opacity)
	{
		if (!filter)
			return;
		obs_data_t *settings = obs_data_create();
		obs_data_set_double(settings, "opacity", opacity);
		obs_source_update(filter, settings);
		obs_data_release(settings);
	}

	obs_source_t *text = nullptr;
	obs_source_t *textOpacity = nullptr;
	obs_source_t *shadowText = nullptr;
	obs_source_t *shadowOpacity = nullptr;
	obs_source_t *media = nullptr;
	obs_source_t *mediaOpacity = nullptr;
	obs_data_t *font = nullptr;
	std::mutex mutex;
	std::deque<std::string> queue;
	std::unordered_map<std::string, std::chrono::steady_clock::time_point> recentlySeen;
	std::string currentNickname;
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
	int opacityStep = -1;
	size_t maxQueue = 20;
	float displaySeconds = 5.0f;
	float fadeInSeconds = 0.3f;
	float fadeOutSeconds = 0.3f;
	float elapsed = 0.0f;
	bool outline = true;
	bool shadow = true;
	bool active = false;
	bool mediaEnabled = false;
	bool mediaKeepAspect = true;
	bool mediaRestart = true;
};

const char *sourceName(void *)
{
	return obs_module_text("Source.Name");
}

void *sourceCreate(obs_data_t *settings, obs_source_t *)
{
	return new AlertSource(settings);
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
	obs_properties_t *properties = obs_properties_create();
	obs_properties_add_button2(properties, "test_alert", obs_module_text("Source.TestAlert"), testButtonClicked,
				   data);

	obs_properties_t *mediaProperties = obs_properties_create();
	obs_properties_add_bool(mediaProperties, kMediaEnabled, obs_module_text("Source.Media.Enabled"));
	obs_properties_add_path(mediaProperties, kMediaFile, obs_module_text("Source.Media.File"), OBS_PATH_FILE,
				"Image Files (*.bmp *.tga *.png *.jpeg *.jpg *.gif *.psd *.webp);;All Files (*.*)",
				nullptr);
	obs_property_t *mediaPosition =
		obs_properties_add_list(mediaProperties, kMediaPosition, obs_module_text("Source.Media.Position"),
					OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(mediaPosition, obs_module_text("Source.Media.Position.Left"), "left");
	obs_property_list_add_string(mediaPosition, obs_module_text("Source.Media.Position.Right"), "right");
	obs_property_list_add_string(mediaPosition, obs_module_text("Source.Media.Position.Top"), "top");
	obs_property_list_add_string(mediaPosition, obs_module_text("Source.Media.Position.Bottom"), "bottom");
	obs_property_list_add_string(mediaPosition, obs_module_text("Source.Media.Position.Background"), "background");
	obs_properties_add_int(mediaProperties, kMediaWidth, obs_module_text("Source.Media.Width"), 1, 3840, 1);
	obs_properties_add_int(mediaProperties, kMediaHeight, obs_module_text("Source.Media.Height"), 1, 2160, 1);
	obs_properties_add_int(mediaProperties, kMediaMargin, obs_module_text("Source.Media.Margin"), 0, 500, 1);
	obs_properties_add_int_slider(mediaProperties, kMediaOpacity, obs_module_text("Source.Media.Opacity"), 0, 100,
				      1);
	obs_properties_add_bool(mediaProperties, kMediaKeepAspect, obs_module_text("Source.Media.KeepAspect"));
	obs_properties_add_bool(mediaProperties, kMediaRestart, obs_module_text("Source.Media.Restart"));
	obs_properties_add_group(properties, "media", obs_module_text("Source.Media.Group"), OBS_GROUP_NORMAL,
				 mediaProperties);

	obs_properties_add_text(properties, kTemplate, obs_module_text("Source.MessageTemplate"), OBS_TEXT_DEFAULT);
	obs_properties_add_font(properties, kFont, obs_module_text("Source.Font"));
	obs_properties_add_color(properties, kTextColor, obs_module_text("Source.TextColor"));
	obs_properties_add_color(properties, kBackgroundColor, obs_module_text("Source.BackgroundColor"));
	obs_properties_add_int_slider(properties, kBackgroundOpacity, obs_module_text("Source.BackgroundOpacity"), 0,
				      100, 1);
	obs_properties_add_bool(properties, kOutline, obs_module_text("Source.Outline"));
	obs_properties_add_int_slider(properties, kOutlineSize, obs_module_text("Source.OutlineSize"), 1, 20, 1);
	obs_properties_add_color(properties, kOutlineColor, obs_module_text("Source.OutlineColor"));
	obs_properties_add_bool(properties, kShadow, obs_module_text("Source.Shadow"));
	obs_properties_add_color(properties, kShadowColor, obs_module_text("Source.ShadowColor"));
	obs_properties_add_int_slider(properties, kShadowOpacity, obs_module_text("Source.ShadowOpacity"), 0, 100, 1);
	obs_properties_add_int_slider(properties, kShadowDistance, obs_module_text("Source.ShadowDistance"), 0, 30, 1);

	obs_property_t *align = obs_properties_add_list(properties, kAlignment, obs_module_text("Source.Alignment"),
						       OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(align, obs_module_text("Source.Alignment.Left"), "left");
	obs_property_list_add_string(align, obs_module_text("Source.Alignment.Center"), "center");
	obs_property_list_add_string(align, obs_module_text("Source.Alignment.Right"), "right");

	obs_properties_add_int(properties, kWidth, obs_module_text("Source.Width"), 100, 3840, 1);
	obs_properties_add_int(properties, kHeight, obs_module_text("Source.Height"), 50, 1080, 1);
	obs_properties_add_float_slider(properties, kDisplaySeconds, obs_module_text("Source.DisplaySeconds"), 0.5,
					30.0, 0.1);
	obs_properties_add_int_slider(properties, kFadeInMs, obs_module_text("Source.FadeIn"), 0, 3000, 50);
	obs_properties_add_int_slider(properties, kFadeOutMs, obs_module_text("Source.FadeOut"), 0, 3000, 50);
	obs_properties_add_int(properties, kMaxQueue, obs_module_text("Source.MaxQueue"), 1, 100, 1);
	obs_properties_add_int(properties, kDedupeSeconds, obs_module_text("Source.DedupeSeconds"), 0, 300, 1);
	return properties;
}

void sourceDefaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, kTemplate, "{nickname}님이 팔로우하였습니다.");
	obs_data_t *font = obs_data_create();
	obs_data_set_default_string(font, "face", "Malgun Gothic");
	obs_data_set_default_string(font, "style", "Bold");
	obs_data_set_default_int(font, "size", 42);
	obs_data_set_default_int(font, "flags", OBS_FONT_BOLD);
	obs_data_set_default_obj(settings, kFont, font);
	obs_data_release(font);
	obs_data_set_default_int(settings, kTextColor, 0xFFFFFF);
	obs_data_set_default_int(settings, kBackgroundColor, 0x14141C);
	obs_data_set_default_int(settings, kBackgroundOpacity, 55);
	obs_data_set_default_bool(settings, kOutline, true);
	obs_data_set_default_int(settings, kOutlineSize, 3);
	obs_data_set_default_int(settings, kOutlineColor, 0x000000);
	obs_data_set_default_bool(settings, kShadow, true);
	obs_data_set_default_int(settings, kShadowColor, 0x000000);
	obs_data_set_default_int(settings, kShadowOpacity, 70);
	obs_data_set_default_int(settings, kShadowDistance, 4);
	obs_data_set_default_string(settings, kAlignment, "center");
	obs_data_set_default_int(settings, kWidth, 800);
	obs_data_set_default_int(settings, kHeight, 200);
	obs_data_set_default_double(settings, kDisplaySeconds, 5.0);
	obs_data_set_default_int(settings, kFadeInMs, 300);
	obs_data_set_default_int(settings, kFadeOutMs, 300);
	obs_data_set_default_int(settings, kMaxQueue, 20);
	obs_data_set_default_int(settings, kDedupeSeconds, 30);
	obs_data_set_default_bool(settings, kMediaEnabled, false);
	obs_data_set_default_string(settings, kMediaFile, "");
	obs_data_set_default_string(settings, kMediaPosition, "left");
	obs_data_set_default_int(settings, kMediaWidth, 160);
	obs_data_set_default_int(settings, kMediaHeight, 160);
	obs_data_set_default_int(settings, kMediaMargin, 20);
	obs_data_set_default_int(settings, kMediaOpacity, 100);
	obs_data_set_default_bool(settings, kMediaKeepAspect, true);
	obs_data_set_default_bool(settings, kMediaRestart, true);
}
} // namespace

void registerChzzkAlertSource()
{
	obs_source_info info = {};
	info.id = kSourceId;
	info.type = OBS_SOURCE_TYPE_INPUT;
	info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW | OBS_SOURCE_SRGB;
	info.icon_type = OBS_ICON_TYPE_TEXT;
	info.get_name = sourceName;
	info.create = sourceCreate;
	info.destroy = sourceDestroy;
	info.update = sourceUpdate;
	info.get_defaults = sourceDefaults;
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
