#include "chzzk-alert-server.hpp"
#include "chzzk-alert-source.hpp"
#include "chzzk-alert-request.hpp"

#include <QHostAddress>
#include <QTimer>
#include <QTcpServer>
#include <QTcpSocket>

namespace {
constexpr qint64 kMaxRequestLineBytes = 16 * 1024;
constexpr int kRequestTimeoutMs = 5000;
constexpr int kResponseDrainMs = 20;

void sendResponse(QTcpSocket *socket, const AlertRequest &request)
{
	socket->setProperty("handled", true);
	socket->readAll();
	socket->write(alertResponse(request));
	socket->flush();
	// Give the peer time to receive a rejection before closing with unread input.
	QTimer::singleShot(kResponseDrainMs, socket, &QTcpSocket::disconnectFromHost);
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
		socket->setReadBufferSize(kMaxRequestLineBytes + 1);
		connect(socket, &QTcpSocket::readyRead, this, &ChzzkAlertServer::readRequest);
		connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
		QTimer::singleShot(kRequestTimeoutMs, socket, &QTcpSocket::abort);
	}
}

void ChzzkAlertServer::readRequest()
{
	auto *socket = qobject_cast<QTcpSocket *>(sender());
	if (!socket)
		return;
	if (socket->property("handled").toBool()) {
		socket->readAll();
		return;
	}
	if (!socket->canReadLine()) {
		if (socket->bytesAvailable() >= kMaxRequestLineBytes)
			sendResponse(socket, {"414 URI Too Long", false, {}});
		return;
	}
	const QByteArray line = socket->readLine(kMaxRequestLineBytes + 1);
	if (!line.endsWith('\n')) {
		sendResponse(socket, {"414 URI Too Long", false, {}});
		return;
	}
	const AlertRequest request = parseAlertRequest(line);
	if (!request.nickname.isEmpty())
		publishFollowToAlertSources(request.nickname);
	sendResponse(socket, request);
}
