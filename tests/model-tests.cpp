#include "check.hpp"
#include "chzzk-alert-model.hpp"
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

	const AlertTiming fade{1.0f, 2.0f, 1.0f};
	CHECK(fade.opacity(0) == 0);
	CHECK(fade.opacity(0.5f) == 0.5f);
	CHECK(fade.opacity(2) == 1);
	CHECK(fade.opacity(3.5f) == 0.5f);
	CHECK(fade.finished(4) && fade.opacity(4) == 0);
	CHECK((AlertTiming{0, 1, 0}.opacity(0) == 1));
	CHECK((AlertTiming{0, 1, 0}.finished(1)));

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
