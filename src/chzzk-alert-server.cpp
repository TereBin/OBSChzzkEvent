#include "chzzk-alert-server.hpp"
#include "chzzk-alert-source.hpp"
#include "chzzk-alert-request.hpp"
#include "chzzk-alert-connection.hpp"

#include <QHostAddress>
#include <QTimer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
void sendResponse(QTcpSocket *socket, const AlertRequest &request)
{
	socket->setProperty("handled", true);
	socket->readAll();
	socket->write(alertResponse(request));
	socket->flush();
	// Give the peer time to receive a rejection before closing with unread input.
	QTimer::singleShot(AlertProtocol::responseDrainMs, socket, &QTcpSocket::disconnectFromHost);
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
	alertConnectionState().setListening(true);
	return true;
}

void ChzzkAlertServer::stop()
{
	server_->close();
	port_ = 0;
	alertConnectionState().setListening(false);
}

void ChzzkAlertServer::acceptConnection()
{
	while (server_->hasPendingConnections()) {
		QTcpSocket *socket = server_->nextPendingConnection();
		socket->setReadBufferSize(AlertProtocol::maxRequestLineBytes + 1);
		connect(socket, &QTcpSocket::readyRead, this, &ChzzkAlertServer::readRequest);
		connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
		QTimer::singleShot(AlertProtocol::requestTimeoutMs, socket, &QTcpSocket::abort);
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
		if (socket->bytesAvailable() >= AlertProtocol::maxRequestLineBytes)
			sendResponse(socket, {"414 URI Too Long", false, {}});
		return;
	}
	const QByteArray line = socket->readLine(AlertProtocol::maxRequestLineBytes + 1);
	if (!line.endsWith('\n')) {
		sendResponse(socket, {"414 URI Too Long", false, {}});
		return;
	}
	AlertRequest request = parseAlertRequest(line);
	handleRequest(request);
	sendResponse(socket, request);
}

void ChzzkAlertServer::handleRequest(AlertRequest &request)
{
	if (request.kind == AlertRequest::Kind::Follow && eventHistory_.accept(request.eventId)) {
		publishFollowToAlertSources(request.nickname);
		alertConnectionState().receivedFollow();
	} else if (request.kind == AlertRequest::Kind::Heartbeat) {
		alertConnectionState().heartbeat(request.session, request.pending, request.error);
	} else if (request.kind == AlertRequest::Kind::Status) {
		const auto state = alertConnectionState().snapshot();
		QJsonObject body{{"listening", state.listening},   {"bridges", state.bridges},
				 {"pending", state.pending},       {"lastContact", state.lastContact},
				 {"lastFollow", state.lastFollow}, {"lastError", state.lastError}};
		request.body = QJsonDocument(body).toJson(QJsonDocument::Compact);
	}
}
