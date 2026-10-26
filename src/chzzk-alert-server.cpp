#include "chzzk-alert-server.hpp"

#include <QJsonDocument>
#include <QHostAddress>
#include <QUrl>
#include <QUrlQuery>
#include <QTcpServer>
#include <QTcpSocket>

#include <utility>

ChzzkAlertServer::ChzzkAlertServer(QObject *parent) : QObject(parent), server_(new QTcpServer(this))
{
	connect(server_, &QTcpServer::newConnection, this, &ChzzkAlertServer::acceptConnection);
}

ChzzkAlertServer::~ChzzkAlertServer()
{
	stop();
}

bool ChzzkAlertServer::start(quint16 port)
{
	if (server_->isListening())
		return true;
	if (!server_->listen(QHostAddress::LocalHost, port))
		return false;
	port_ = server_->serverPort();
	return true;
}

void ChzzkAlertServer::stop()
{
	for (QTcpSocket *client : std::as_const(clients_))
		client->disconnectFromHost();
	clients_.clear();
	server_->close();
	port_ = 0;
}

void ChzzkAlertServer::acceptConnection()
{
	while (server_->hasPendingConnections()) {
		QTcpSocket *socket = server_->nextPendingConnection();
		connect(socket, &QTcpSocket::readyRead, this, &ChzzkAlertServer::readRequest);
		connect(socket, &QTcpSocket::disconnected, this, &ChzzkAlertServer::removeClient);
	}
}

void ChzzkAlertServer::readRequest()
{
	auto *socket = qobject_cast<QTcpSocket *>(sender());
	if (!socket || !socket->canReadLine())
		return;

	const QByteArray requestLine = socket->readLine();
	while (socket->canReadLine())
		socket->readLine();

	const QList<QByteArray> requestParts = requestLine.trimmed().split(' ');
	if (requestParts.size() < 2) {
		socket->disconnectFromHost();
		return;
	}

	const QUrl requestUrl(QString::fromUtf8(requestParts.at(1)));
	if (requestUrl.path() == QStringLiteral("/publish")) {
		const QUrlQuery query(requestUrl);
		const QString type = query.queryItemValue(QStringLiteral("type"));
		const QString nickname = query.queryItemValue(QStringLiteral("nickname"));
		if (type == QStringLiteral("follow"))
			publishFollow(nickname);
		socket->write("HTTP/1.1 204 No Content\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n");
		socket->disconnectFromHost();
		return;
	}

	if (requestUrl.path() != QStringLiteral("/events")) {
		socket->write("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n");
		socket->disconnectFromHost();
		return;
	}

	sendHeaders(socket);
	clients_.insert(socket);
}

void ChzzkAlertServer::removeClient()
{
	if (auto *socket = qobject_cast<QTcpSocket *>(sender()))
		clients_.remove(socket);
}

void ChzzkAlertServer::sendHeaders(QTcpSocket *socket)
{
	socket->write("HTTP/1.1 200 OK\r\n"
		      "Content-Type: text/event-stream\r\n"
		      "Cache-Control: no-cache\r\n"
		      "Connection: keep-alive\r\n"
		      "Access-Control-Allow-Origin: *\r\n\r\n");
	socket->flush();
}

void ChzzkAlertServer::publish(const QJsonObject &event)
{
	const QByteArray payload = "data: " + QJsonDocument(event).toJson(QJsonDocument::Compact) + "\n\n";
	for (QTcpSocket *client : std::as_const(clients_)) {
		if (client->state() == QAbstractSocket::ConnectedState) {
			client->write(payload);
			client->flush();
		}
	}
}

void ChzzkAlertServer::publishFollow(const QString &nickname)
{
	publish({{QStringLiteral("type"), QStringLiteral("follow")},
		 {QStringLiteral("nickname"), nickname}});
}

