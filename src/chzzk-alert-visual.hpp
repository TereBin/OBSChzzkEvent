#pragma once

#include "chzzk-obs-resource.hpp"

// Owns a private visual source and its opacity filter as one resource.
class AlertVisual {
public:
	AlertVisual(const char *id, const char *name, const char *filterName)
		: source_(obs_source_create_private(id, name, nullptr))
	{
		if (source_) {
			filter_.reset(obs_source_create_private("color_filter", filterName, nullptr));
			if (filter_)
				obs_source_filter_add(source_.get(), filter_.get());
		}
	}
	~AlertVisual()
	{
		if (source_ && filter_)
			obs_source_filter_remove(source_.get(), filter_.get());
	}
	AlertVisual(const AlertVisual &) = delete;
	AlertVisual &operator=(const AlertVisual &) = delete;
	obs_source_t *get() const { return source_.get(); }
	explicit operator bool() const { return bool(source_); }
	void setOpacity(double opacity)
	{
		if (!filter_)
			return;
		auto settings = makeObsData();
		obs_data_set_double(settings.get(), "opacity", opacity);
		obs_source_update(filter_.get(), settings.get());
	}

private:
	ObsSourcePtr source_;
	ObsSourcePtr filter_;
};
