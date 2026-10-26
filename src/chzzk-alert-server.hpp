#pragma once

#include <QJsonObject>
#include <QObject>
#include <QSet>
#include <QString>

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
	void publish(const QJsonObject &event);
	void publishFollow(const QString &nickname);

private slots:
	void acceptConnection();
	void readRequest();
	void removeClient();

private:
	void sendHeaders(QTcpSocket *socket);

	QTcpServer *server_ = nullptr;
	QSet<QTcpSocket *> clients_;
	quint16 port_ = 0;
};
