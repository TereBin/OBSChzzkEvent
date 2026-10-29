#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>

// The owner serializes queue mutations; timestamps are injectable for deterministic tests.
class AlertQueue {
public:
	using Clock = std::chrono::steady_clock;

	void configure(size_t capacity, int dedupeSeconds)
	{
		capacity_ = std::max(size_t{1}, capacity);
		dedupeSeconds_ = std::max(0, dedupeSeconds);
		while (pending_.size() > capacity_)
			pending_.pop_front();
	}

	bool enqueue(const std::string &nickname, bool test = false, Clock::time_point now = Clock::now())
	{
		if (nickname.empty())
			return false;
		prune(now);
		if (!test && dedupeSeconds_ > 0) {
			const auto found = recent_.find(nickname);
			if (found != recent_.end() && now - found->second < std::chrono::seconds(dedupeSeconds_))
				return false;
			recent_[nickname] = now;
		}
		if (pending_.size() >= capacity_)
			pending_.pop_front();
		pending_.push_back(nickname);
		return true;
	}

	bool empty() const { return pending_.empty(); }
	std::string pop()
	{
		if (empty())
			return {};
		std::string next = std::move(pending_.front());
		pending_.pop_front();
		return next;
	}

private:
	void prune(Clock::time_point now)
	{
		const auto retention = std::chrono::seconds(std::max(dedupeSeconds_, 300));
		for (auto it = recent_.begin(); it != recent_.end();) {
			if (now - it->second > retention)
				it = recent_.erase(it);
			else
				++it;
		}
	}
	size_t capacity_ = 20;
	int dedupeSeconds_ = 30;
	std::deque<std::string> pending_;
	std::unordered_map<std::string, Clock::time_point> recent_;
};

struct AlertTiming {
	float fadeIn;
	float display;
	float fadeOut;
	bool finished(float elapsed) const { return elapsed >= fadeIn + display + fadeOut; }
	float opacity(float elapsed) const
	{
		if (finished(elapsed))
			return 0.0f;
		if (fadeIn > 0.0f && elapsed < fadeIn)
			return std::clamp(elapsed / fadeIn, 0.0f, 1.0f);
		const float fadeOutStart = fadeIn + display;
		if (fadeOut > 0.0f && elapsed > fadeOutStart)
			return std::clamp((fadeOutStart + fadeOut - elapsed) / fadeOut, 0.0f, 1.0f);
		return 1.0f;
	}
};

class AlertSceneGate {
public:
	enum class Action { Hidden, Pause, Visible, Resume };
	Action update(bool visible, bool playing)
	{
		if (!visible) {
			if (playing && !suspended_) {
				suspended_ = true;
				return Action::Pause;
			}
			return Action::Hidden;
		}
		if (suspended_) {
			suspended_ = false;
			return Action::Resume;
		}
		return Action::Visible;
	}

private:
	bool suspended_ = false;
};

// Size callbacks may be re-entered from OBS signals while the owner's mutex is held.
class AlertSizeSnapshot {
public:
	void update(uint32_t width, uint32_t height, uint32_t padding)
	{
		value_.store((uint64_t{width + padding} << 32) | (height + padding), std::memory_order_relaxed);
	}
	uint32_t width() const { return static_cast<uint32_t>(value_.load(std::memory_order_relaxed) >> 32); }
	uint32_t height() const { return static_cast<uint32_t>(value_.load(std::memory_order_relaxed)); }

private:
	std::atomic<uint64_t> value_{(uint64_t{804} << 32) | 204};
};

inline std::string formatAlertMessage(const std::string &format, const std::string &nickname)
{
	constexpr auto token = "{nickname}";
	constexpr size_t tokenLength = 10;
	std::string result = format;
	size_t position = 0;
	while ((position = result.find(token, position)) != std::string::npos) {
		result.replace(position, tokenLength, nickname);
		position += nickname.size();
	}
	return result;
}
