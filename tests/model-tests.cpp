#include "check.hpp"
#include "chzzk-alert-model.hpp"
#include "chzzk-alert-layout.hpp"
#include <cmath>
#include <mutex>

int main()
{
	using namespace std::chrono_literals;
	AlertQueue queue;
	const auto now = AlertQueue::Clock::time_point{};
	queue.configure(2, 30);
	CHECK(!queue.enqueue("", false, now));
	CHECK(queue.enqueue("first", false, now));
	CHECK(!queue.enqueue("first", false, now + 29s));
	CHECK(queue.enqueue("second", false, now));
	CHECK(queue.enqueue("third", false, now));
	CHECK(queue.pop() == "second");
	CHECK(queue.pop() == "third");
	CHECK(queue.empty());
	CHECK(queue.enqueue("first", false, now + 30s));
	CHECK(queue.enqueue("first", true, now + 30s));
	queue.configure(1, 0);
	CHECK(queue.pop() == "first");
	CHECK(queue.empty());
	CHECK(queue.enqueue("first", false, now + 31s));
	CHECK(queue.enqueue("first", false, now + 31s));
	CHECK(queue.pop() == "first" && queue.empty());
	CHECK(queue.pop().empty());
	AlertSceneGate gate;
	CHECK(queue.enqueue("hidden follow", true));
	for (int i = 0; i < 100; ++i)
		CHECK(gate.update(false, false) == AlertSceneGate::Action::Hidden);
	CHECK(!queue.empty());
	CHECK(gate.update(true, false) == AlertSceneGate::Action::Visible);
	CHECK(queue.pop() == "hidden follow");
	CHECK(gate.update(false, true) == AlertSceneGate::Action::Pause);
	CHECK(gate.update(false, true) == AlertSceneGate::Action::Hidden);
	CHECK(gate.update(true, true) == AlertSceneGate::Action::Resume);
	CHECK(gate.update(true, true) == AlertSceneGate::Action::Visible);

	const AlertTiming fade{1.0f, 2.0f, 1.0f};
	CHECK(fade.opacity(0) == 0);
	CHECK(fade.opacity(0.5f) == 0.5f);
	CHECK(fade.opacity(2) == 1);
	CHECK(fade.opacity(3.5f) == 0.5f);
	CHECK(fade.finished(4) && fade.opacity(4) == 0);
	CHECK((AlertTiming{0, 1, 0}.opacity(0) == 1));
	CHECK((AlertTiming{0, 1, 0}.finished(1)));

	const auto textOnly = calculateAlertLayout(800, 200, 0, 0, 20, "left");
	CHECK(textOnly.textWidth == 800 && textOnly.textHeight == 200);
	const auto left = calculateAlertLayout(800, 200, 160, 160, 20, "left");
	CHECK(left.textX == 180 && left.textWidth == 620 && left.mediaY == 20);
	const auto right = calculateAlertLayout(800, 200, 160, 160, 20, "right");
	CHECK(right.textX == 0 && right.mediaX == 640 && right.textWidth == 620);
	const auto top = calculateAlertLayout(800, 200, 160, 160, 20, "top");
	CHECK(top.textY == 180 && top.textHeight == 20 && top.mediaX == 320);
	const auto bottom = calculateAlertLayout(800, 200, 160, 160, 20, "bottom");
	CHECK(bottom.textY == 0 && bottom.mediaY == 40 && bottom.textHeight == 20);
	const auto oversized = calculateAlertLayout(100, 50, 160, 160, 500, "left");
	CHECK(oversized.textWidth == 1 && oversized.mediaWidth == 100 && oversized.mediaHeight == 50);
	const auto background = calculateAlertLayout(800, 200, 160, 160, 20, "background");
	CHECK(background.textWidth == 800 && background.mediaWidth == 800 && background.mediaHeight == 200);

	AlertSizeSnapshot dimensions;
	std::mutex ownerMutex;
	{
		std::lock_guard lock(ownerMutex);
		dimensions.update(800, 200, 4);
		// Same-thread OBS signal re-entry must be safe while the owner's lock is held.
		CHECK(dimensions.width() == 804 && dimensions.height() == 204);
	}
	dimensions.update(100, 50, 0);
	CHECK(dimensions.width() == 100 && dimensions.height() == 50);
	CHECK(formatAlertMessage("{nickname}/{nickname}", "Test") == "Test/Test");
	CHECK(formatAlertMessage("{nickname}", "{nickname}") == "{nickname}");
	CHECK(formatAlertMessage("unchanged", "Test") == "unchanged");
	std::cout << "Alert model checks passed\n";
}
