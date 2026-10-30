#include "chzzk-alert-server.hpp"
#include "chzzk-alert-source.hpp"
#include "chzzk-alert-status-dock.hpp"

#include <obs-module.h>
#include <plugin-support.h>

#include <memory>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")
OBS_MODULE_AUTHOR("OBSChzzkEvent contributors")

namespace {
std::unique_ptr<ChzzkAlertServer> g_alertServer;
} // namespace

const char *obs_module_name(void)
{
	return obs_module_text("Plugin.Name");
}

const char *obs_module_description(void)
{
	return "Displays CHZZK follow events with text, images and sound in a native OBS source.";
}

bool obs_module_load(void)
{
	registerChzzkAlertSource();
	g_alertServer = std::make_unique<ChzzkAlertServer>();
	if (!g_alertServer->start()) {
		obs_log(LOG_ERROR, "could not listen on 127.0.0.1:%u", AlertProtocol::port);
		g_alertServer.reset();
		return false;
	}

	obs_log(LOG_INFO, "plugin loaded (version %s, port %u)", PLUGIN_VERSION, AlertProtocol::port);
	registerAlertStatusDock();
	return true;
}

void obs_module_unload(void)
{
	unregisterAlertStatusDock();
	g_alertServer.reset();
	obs_log(LOG_INFO, "plugin unloaded");
}
