#pragma once

#include <QByteArray>
#include <QString>

struct AlertRequest {
	enum class Kind { Invalid, Follow, Test, Heartbeat, Status, Preflight };
	const char *status = "400 Bad Request";
	bool cors = false;
	QString nickname;
	Kind kind = Kind::Invalid;
	bool isAlert() const { return kind == Kind::Follow || kind == Kind::Test; }
	bool isTest() const { return kind == Kind::Test; }
	QString eventId;
	QString session;
	int pending = 0;
	QString error;
	QByteArray body;
};

AlertRequest parseAlertRequest(const QByteArray &requestLine);
QByteArray alertResponse(const AlertRequest &request);
