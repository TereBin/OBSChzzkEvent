#include "chzzk-alert-server.hpp"

#include <obs-module.h>
#include <plugin-support.h>

#include <memory>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")
OBS_MODULE_AUTHOR("OBS Chzzk Alerts contributors")

namespace {
constexpr quint16 kAlertServerPort = 21345;
std::unique_ptr<ChzzkAlertServer> g_alertServer;
} // namespace

const char *obs_module_description(void)
{
	return "Relays CHZZK remote-control follow events to an OBS browser source.";
}

bool obs_module_load(void)
{
	g_alertServer = std::make_unique<ChzzkAlertServer>();
	if (!g_alertServer->start(kAlertServerPort)) {
		obs_log(LOG_ERROR, "could not listen on 127.0.0.1:%u", kAlertServerPort);
		g_alertServer.reset();
		return false;
	}

	obs_log(LOG_INFO, "plugin loaded (version %s, port %u)", PLUGIN_VERSION, kAlertServerPort);
	return true;
}

void obs_module_unload(void)
{
	g_alertServer.reset();
	obs_log(LOG_INFO, "plugin unloaded");
}
