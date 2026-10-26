#pragma once

#include <QObject>

class QTcpServer;
class QTcpSocket;

class ChzzkAlertServer final : public QObject {
	Q_OBJECT

public:
	explicit ChzzkAlertServer(QObject *parent = nullptr);
	~ChzzkAlertServer() override;

	bool start(quint16 port = 21345);
	void stop();
	quint16 port() const { return port_; }
private slots:
	void acceptConnection();
	void readRequest();

private:
	QTcpServer *server_ = nullptr;
	quint16 port_ = 0;
};
