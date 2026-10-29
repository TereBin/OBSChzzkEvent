#pragma once

#include <QDateTime>
#include <QHash>
#include <QString>
#include <mutex>

struct AlertConnectionSnapshot {
	bool listening = false;
	int bridges = 0;
	int pending = 0;
	qint64 lastContact = 0;
	qint64 lastFollow = 0;
	QString lastError;
};

class AlertConnectionState {
public:
	void setListening(bool listening);
	void heartbeat(const QString &session, int pending, const QString &error,
		       qint64 now = QDateTime::currentMSecsSinceEpoch());
	void receivedFollow(qint64 now = QDateTime::currentMSecsSinceEpoch());
	AlertConnectionSnapshot snapshot(qint64 now = QDateTime::currentMSecsSinceEpoch()) const;

private:
	struct Bridge {
		qint64 contact;
		int pending;
	};
	mutable std::mutex mutex_;
	bool listening_ = false;
	QHash<QString, Bridge> bridges_;
	qint64 lastContact_ = 0;
	qint64 lastFollow_ = 0;
	QString lastError_;
};

AlertConnectionState &alertConnectionState();
