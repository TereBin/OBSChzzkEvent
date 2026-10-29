#include "chzzk-alert-connection.hpp"
#include "chzzk-alert-protocol.hpp"

#include <algorithm>

void AlertConnectionState::setListening(bool listening)
{
	std::lock_guard lock(mutex_);
	listening_ = listening;
	if (!listening)
		bridges_.clear();
}

void AlertConnectionState::heartbeat(const QString &session, int pending, const QString &error, qint64 now)
{
	std::lock_guard lock(mutex_);
	for (auto it = bridges_.begin(); it != bridges_.end();) {
		if (now - it->contact > AlertProtocol::bridgeTimeoutMs)
			it = bridges_.erase(it);
		else
			++it;
	}
	if (!bridges_.contains(session) && bridges_.size() >= AlertProtocol::maxBridges) {
		auto oldest = std::min_element(bridges_.begin(), bridges_.end(),
					       [](const Bridge &a, const Bridge &b) { return a.contact < b.contact; });
		bridges_.erase(oldest);
	}
	bridges_.insert(session, {now, std::clamp(pending, 0, AlertProtocol::maxPending)});
	lastContact_ = now;
	if (!error.isEmpty())
		lastError_ = error;
}

void AlertConnectionState::receivedFollow(qint64 now)
{
	std::lock_guard lock(mutex_);
	lastFollow_ = now;
}

AlertConnectionSnapshot AlertConnectionState::snapshot(qint64 now) const
{
	std::lock_guard lock(mutex_);
	AlertConnectionSnapshot result;
	result.listening = listening_;
	result.lastFollow = lastFollow_;
	result.lastError = lastError_;
	result.lastContact = lastContact_;
	for (const auto &bridge : bridges_) {
		if (listening_ && now - bridge.contact <= AlertProtocol::bridgeTimeoutMs) {
			++result.bridges;
			result.pending += bridge.pending;
		}
	}
	return result;
}

AlertConnectionState &alertConnectionState()
{
	static AlertConnectionState state;
	return state;
}
