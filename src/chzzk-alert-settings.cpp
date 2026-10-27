#include "chzzk-alert-settings.hpp"

#include <obs-module.h>
#include <algorithm>

namespace {
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
constexpr auto kSoundEnabled = "sound_enabled";
constexpr auto kSoundPreset = "sound_preset";
constexpr auto kSoundFile = "sound_file";
constexpr auto kSoundVolume = "sound_volume";

constexpr auto kSoundPresetAlarm1 = "alarm1";
constexpr auto kSoundPresetAlarm2 = "alarm2";
constexpr auto kSoundPresetAlarm3 = "alarm3";
constexpr auto kSoundPresetCustom = "custom";

std::string moduleFilePath(const char *relativePath)
{
	char *path = obs_module_file(relativePath);
	if (!path)
		return {};
	std::string result = path;
	bfree(path);
	return result;
}

std::string resolveSoundFile(obs_data_t *settings)
{
	const char *customFile = obs_data_get_string(settings, kSoundFile);
	if (!obs_data_has_user_value(settings, kSoundPreset) && customFile && *customFile)
		return customFile;

	const std::string preset = obs_data_get_string(settings, kSoundPreset);
	if (preset == kSoundPresetCustom)
		return customFile ? customFile : "";
	if (preset == kSoundPresetAlarm2)
		return moduleFilePath("sounds/Alarm_2.mp3");
	if (preset == kSoundPresetAlarm3)
		return moduleFilePath("sounds/Alarm_3.mp3");
	return moduleFilePath("sounds/Alarm_1.mp3");
}

} // namespace

AlertSettings readAlertSettings(obs_data_t *settings)
{
	AlertSettings result;
	result.messageTemplate = obs_data_get_string(settings, kTemplate);
	result.textColor = static_cast<uint32_t>(obs_data_get_int(settings, kTextColor));
	result.backgroundColor = static_cast<uint32_t>(obs_data_get_int(settings, kBackgroundColor));
	result.backgroundOpacity =
		static_cast<int>(std::clamp<int64_t>(obs_data_get_int(settings, kBackgroundOpacity), 0, 100));
	result.outline = obs_data_get_bool(settings, kOutline);
	result.outlineSize = static_cast<int>(std::clamp<int64_t>(obs_data_get_int(settings, kOutlineSize), 1, 20));
	result.outlineColor = static_cast<uint32_t>(obs_data_get_int(settings, kOutlineColor));
	result.shadow = obs_data_get_bool(settings, kShadow);
	result.shadowColor = static_cast<uint32_t>(obs_data_get_int(settings, kShadowColor));
	result.shadowOpacityPercent =
		static_cast<int>(std::clamp<int64_t>(obs_data_get_int(settings, kShadowOpacity), 0, 100));
	result.shadowDistance =
		static_cast<int>(std::clamp<int64_t>(obs_data_get_int(settings, kShadowDistance), 0, 30));
	result.alignment = obs_data_get_string(settings, kAlignment);
	result.width = static_cast<uint32_t>(std::clamp<int64_t>(obs_data_get_int(settings, kWidth), 100, 3840));
	result.height = static_cast<uint32_t>(std::clamp<int64_t>(obs_data_get_int(settings, kHeight), 50, 1080));
	result.displaySeconds =
		static_cast<float>(std::clamp(obs_data_get_double(settings, kDisplaySeconds), 0.5, 30.0));
	result.fadeInSeconds =
		static_cast<float>(std::clamp<int64_t>(obs_data_get_int(settings, kFadeInMs), 0, 3000)) / 1000.0f;
	result.fadeOutSeconds =
		static_cast<float>(std::clamp<int64_t>(obs_data_get_int(settings, kFadeOutMs), 0, 3000)) / 1000.0f;
	result.maxQueue = static_cast<size_t>(std::clamp<int64_t>(obs_data_get_int(settings, kMaxQueue), 1, 100));
	result.dedupeSeconds =
		static_cast<int>(std::clamp<int64_t>(obs_data_get_int(settings, kDedupeSeconds), 0, 300));
	result.mediaEnabled = obs_data_get_bool(settings, kMediaEnabled);
	result.mediaFile = obs_data_get_string(settings, kMediaFile);
	result.mediaPosition = obs_data_get_string(settings, kMediaPosition);
	result.mediaWidth =
		static_cast<uint32_t>(std::clamp<int64_t>(obs_data_get_int(settings, kMediaWidth), 1, 3840));
	result.mediaHeight =
		static_cast<uint32_t>(std::clamp<int64_t>(obs_data_get_int(settings, kMediaHeight), 1, 2160));
	result.mediaMargin =
		static_cast<uint32_t>(std::clamp<int64_t>(obs_data_get_int(settings, kMediaMargin), 0, 500));
	result.mediaOpacityPercent =
		static_cast<int>(std::clamp<int64_t>(obs_data_get_int(settings, kMediaOpacity), 0, 100));
	result.mediaKeepAspect = obs_data_get_bool(settings, kMediaKeepAspect);
	result.mediaRestart = obs_data_get_bool(settings, kMediaRestart);

	result.font.reset(obs_data_get_obj(settings, kFont));
	result.soundEnabled = obs_data_get_bool(settings, kSoundEnabled);
	result.soundFile = resolveSoundFile(settings);
	result.soundVolume = static_cast<int>(std::clamp<int64_t>(obs_data_get_int(settings, kSoundVolume), 0, 100));
	return result;
}

static bool soundPresetModified(obs_properties_t *properties, obs_property_t *, obs_data_t *settings)
{
	obs_property_t *customFile = obs_properties_get(properties, kSoundFile);
	if (!customFile)
		return false;
	const char *preset = obs_data_get_string(settings, kSoundPreset);
	const bool customSelected = preset && std::string(preset) == kSoundPresetCustom;
	obs_property_set_visible(customFile, customSelected);
	return true;
}

obs_properties_t *createAlertProperties(void *data, obs_property_clicked_t testButtonClicked)
{
	obs_properties_t *properties = obs_properties_create();
	obs_properties_add_button2(properties, "test_alert", obs_module_text("Source.TestAlert"), testButtonClicked,
				   data);

	obs_properties_t *mediaProperties = obs_properties_create();
	obs_properties_add_bool(mediaProperties, kMediaEnabled, obs_module_text("Source.Media.Enabled"));
	obs_properties_add_path(mediaProperties, kMediaFile, obs_module_text("Source.Media.File"), OBS_PATH_FILE,
				"Image Files (*.bmp *.tga *.png *.jpeg *.jpg *.gif *.psd *.webp);;All Files (*.*)",
				nullptr);
	obs_property_t *mediaPosition = obs_properties_add_list(mediaProperties, kMediaPosition,
								obs_module_text("Source.Media.Position"),
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

	obs_properties_t *soundProperties = obs_properties_create();
	obs_properties_add_bool(soundProperties, kSoundEnabled, obs_module_text("Source.Sound.Enabled"));
	obs_property_t *soundPreset = obs_properties_add_list(soundProperties, kSoundPreset,
							      obs_module_text("Source.Sound.Preset"),
							      OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(soundPreset, obs_module_text("Source.Sound.Preset.Alarm1"), kSoundPresetAlarm1);
	obs_property_list_add_string(soundPreset, obs_module_text("Source.Sound.Preset.Alarm2"), kSoundPresetAlarm2);
	obs_property_list_add_string(soundPreset, obs_module_text("Source.Sound.Preset.Alarm3"), kSoundPresetAlarm3);
	obs_property_list_add_string(soundPreset, obs_module_text("Source.Sound.Preset.Custom"), kSoundPresetCustom);
	obs_property_set_modified_callback(soundPreset, soundPresetModified);
	obs_properties_add_path(soundProperties, kSoundFile, obs_module_text("Source.Sound.File"), OBS_PATH_FILE,
				"Audio Files (*.mp3 *.wav *.ogg *.aac *.m4a *.flac *.opus);;All Files (*.*)", nullptr);
	obs_properties_add_int_slider(soundProperties, kSoundVolume, obs_module_text("Source.Sound.Volume"), 0, 100, 1);
	obs_properties_add_group(properties, "sound", obs_module_text("Source.Sound.Group"), OBS_GROUP_NORMAL,
				 soundProperties);

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

void setAlertDefaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, kTemplate, "{nickname}님이 팔로우하였습니다.");
	auto font = makeObsData();
	obs_data_set_default_string(font.get(), "face", "Malgun Gothic");
	obs_data_set_default_string(font.get(), "style", "Bold");
	obs_data_set_default_int(font.get(), "size", 42);
	obs_data_set_default_int(font.get(), "flags", OBS_FONT_BOLD);
	obs_data_set_default_obj(settings, kFont, font.get());
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
	obs_data_set_default_bool(settings, kSoundEnabled, false);
	obs_data_set_default_string(settings, kSoundPreset, kSoundPresetAlarm1);
	obs_data_set_default_string(settings, kSoundFile, "");
	obs_data_set_default_int(settings, kSoundVolume, 100);
}
