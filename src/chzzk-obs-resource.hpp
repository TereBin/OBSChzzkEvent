#pragma once

#include <obs.h>
#include <memory>

struct ObsDataDeleter {
	void operator()(obs_data_t *data) const { obs_data_release(data); }
};
using ObsDataPtr = std::unique_ptr<obs_data_t, ObsDataDeleter>;
inline ObsDataPtr makeObsData()
{
	return ObsDataPtr(obs_data_create());
}

struct ObsSourceDeleter {
	void operator()(obs_source_t *source) const { obs_source_release(source); }
};
using ObsSourcePtr = std::unique_ptr<obs_source_t, ObsSourceDeleter>;
