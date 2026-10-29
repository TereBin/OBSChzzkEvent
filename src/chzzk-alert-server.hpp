#pragma once

#include <QObject>
#include "chzzk-alert-event-history.hpp"
#include "chzzk-alert-protocol.hpp"

class QTcpServer;
class QTcpSocket;
struct AlertRequest;

class ChzzkAlertServer final : public QObject {
	Q_OBJECT

public:
	explicit ChzzkAlertServer(QObject *parent = nullptr);
	~ChzzkAlertServer() override;

	bool start(quint16 port = AlertProtocol::port);
	void stop();
	quint16 port() const { return port_; }
private slots:
	void acceptConnection();
	void readRequest();

private:
	QTcpServer *server_ = nullptr;
	quint16 port_ = 0;
	AlertEventHistory eventHistory_;
	void handleRequest(AlertRequest &request);
};
