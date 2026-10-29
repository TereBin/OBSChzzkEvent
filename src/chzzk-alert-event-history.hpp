#pragma once

#include <QQueue>
#include <QSet>
#include <QString>
#include <chrono>

// Owned by the HTTP server thread. Use monotonic time for retry deduplication.
class AlertEventHistory {
public:
	using Clock = std::chrono::steady_clock;
	static constexpr int capacity = 4096;
	static constexpr auto retention = std::chrono::minutes(10);

	bool accept(const QString &id, Clock::time_point now = Clock::now())
	{
		if (id.isEmpty())
			return true; // Older extensions and manual requests have no event ID.
		while (!order_.isEmpty() && now - order_.head().received >= retention)
			removeOldest();
		if (ids_.contains(id))
			return false;
		if (order_.size() >= capacity)
			removeOldest();
		ids_.insert(id);
		order_.enqueue({id, now});
		return true;
	}

private:
	struct Entry {
		QString id;
		Clock::time_point received;
	};
	void removeOldest() { ids_.remove(order_.dequeue().id); }
	QSet<QString> ids_;
	QQueue<Entry> order_;
};
