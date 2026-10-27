#pragma once

#include <QByteArray>
#include <QString>

struct AlertRequest {
	const char *status = "400 Bad Request";
	bool cors = false;
	QString nickname;
};

AlertRequest parseAlertRequest(const QByteArray &requestLine);
QByteArray alertResponse(const AlertRequest &request);
