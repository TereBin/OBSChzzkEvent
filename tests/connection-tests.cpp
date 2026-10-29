#include "check.hpp"
#include "chzzk-alert-connection.hpp"
#include "chzzk-alert-event-history.hpp"
#include "chzzk-alert-protocol.hpp"

int main()
{
	using namespace std::chrono_literals;
	const auto now = AlertEventHistory::Clock::time_point{};
	AlertEventHistory history;
	CHECK(history.accept({}, now) && history.accept({}, now));
	for (int i = 0; i < AlertEventHistory::capacity; ++i)
		CHECK(history.accept(QString::number(i), now));
	CHECK(!history.accept("0", now)); // Duplicate at capacity cannot evict the oldest ID.
	CHECK(!history.accept("0", now + 1s));
	CHECK(history.accept("next", now + 2s));
	CHECK(history.accept("0", now + 3s)); // Oldest ID was evicted by a new event.
	CHECK(!history.accept("next", now + 601s));
	CHECK(history.accept("next", now + 602s)); // Exact TTL boundary uses monotonic time.

	AlertConnectionState state;
	state.setListening(true);
	state.heartbeat("one", 2, "connection", 1000);
	state.heartbeat("two", 3, {}, 2000);
	state.receivedFollow(2500);
	CHECK(state.snapshot(3000).bridges == 2 && state.snapshot(3000).pending == 5);
	CHECK(state.snapshot(3000).lastFollow == 2500);
	CHECK(state.snapshot(92000).bridges == 1);
	CHECK(state.snapshot(92001).bridges == 0);
	state.heartbeat("new", 999, {}, 93000);
	CHECK(state.snapshot(93000).pending == AlertProtocol::maxPending);
	CHECK(state.snapshot(93000).lastError == "connection");
	state.setListening(false);
	CHECK(!state.snapshot().listening && state.snapshot().bridges == 0);
	CHECK(state.snapshot().lastContact == 93000); // Last response survives disconnect and pruning.
	state.setListening(true);
	for (int i = 0; i <= AlertProtocol::maxBridges; ++i)
		state.heartbeat(QString::number(i), 1, {}, 100000 + i);
	CHECK(state.snapshot(100100).bridges == AlertProtocol::maxBridges);
	CHECK(state.snapshot(100100).pending == AlertProtocol::maxBridges);
	std::cout << "Connection state and bounded event history checks passed\n";
}
