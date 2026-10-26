#include "chzzk-alert-server.hpp"
#include "chzzk-alert-source.hpp"

#include <QHostAddress>
#include <QUrl>
#include <QUrlQuery>
#include <QTcpServer>
#include <QTcpSocket>

namespace {
constexpr auto kCorsHeader = "Access-Control-Allow-Origin: *\r\n";

void sendResponse(QTcpSocket *socket, const char *status, bool cors = false)
{
	QByteArray response = "HTTP/1.1 ";
	response += status;
	response += "\r\n";
	if (cors)
		response += kCorsHeader;
	response += "Content-Length: 0\r\nConnection: close\r\n\r\n";
	socket->write(response);
	socket->disconnectFromHost();
}
} // namespace

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
	server_->close();
	port_ = 0;
}

void ChzzkAlertServer::acceptConnection()
{
	while (server_->hasPendingConnections()) {
		QTcpSocket *socket = server_->nextPendingConnection();
		connect(socket, &QTcpSocket::readyRead, this, &ChzzkAlertServer::readRequest);
		connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
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
		sendResponse(socket, "400 Bad Request");
		return;
	}

	const QByteArray &method = requestParts.at(0);
	const QUrl requestUrl(QString::fromUtf8(requestParts.at(1)));
	if (method == "GET" && requestUrl.path() == QStringLiteral("/publish")) {
		const QUrlQuery query(requestUrl);
		const QString type = query.queryItemValue(QStringLiteral("type"));
		const QString nickname = query.queryItemValue(QStringLiteral("nickname")).trimmed();
		if (type == QStringLiteral("follow") && !nickname.isEmpty()) {
			publishFollowToAlertSources(nickname);
			sendResponse(socket, "204 No Content", true);
		} else {
			sendResponse(socket, "400 Bad Request", true);
		}
		return;
	}

	sendResponse(socket, "404 Not Found");
}

