#include "check.hpp"
#include "chzzk-alert-request.hpp"
#include "chzzk-alert-server.hpp"
#include "chzzk-alert-connection.hpp"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QTcpSocket>

static int published = 0;
static QString received;
void publishFollowToAlertSources(const QString &nickname)
{
	++published;
	received = nickname;
}

template<typename Predicate> void until(const char *label, Predicate predicate)
{
	QElapsedTimer timer;
	timer.start();
	while (!predicate() && timer.elapsed() < 2000)
		QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
	if (!predicate()) {
		std::cerr << "Timed out: " << label << '\n';
		CHECK(false);
	}
}

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	const auto valid = parseAlertRequest("GET /publish?type=follow&nickname=Test%20User HTTP/1.1\r\n");
	CHECK(valid.nickname == "Test User" && valid.cors);
	CHECK(alertResponse(valid).startsWith("HTTP/1.1 204 No Content\r\n"));
	CHECK(alertResponse(valid).contains("Content-Length: 0\r\nConnection: close\r\n\r\n"));
	CHECK(parseAlertRequest("GET /publish?type=follow&nickname=Test+User%2B HTTP/1.1\r\n").nickname ==
	      "Test User+");
	CHECK(parseAlertRequest("GET /publish?type=follow&nickname=%20 HTTP/1.1\r\n").nickname.isEmpty());
	CHECK(parseAlertRequest("GET /publish?type=donation&nickname=x HTTP/1.1\r\n").nickname.isEmpty());
	CHECK(QByteArray(parseAlertRequest("GET /unknown HTTP/1.1\r\n").status).startsWith("404"));
	CHECK(QByteArray(parseAlertRequest("garbage\r\n").status).startsWith("400"));
	CHECK(parseAlertRequest("OPTIONS /publish HTTP/1.1\r\n").kind == AlertRequest::Kind::Preflight);
	CHECK(alertResponse(parseAlertRequest("OPTIONS /heartbeat HTTP/1.1\r\n"))
		      .contains("Allow-Private-Network: true"));
	CHECK(parseAlertRequest("GET /heartbeat?session=browser&pending=8&error=http HTTP/1.1\r\n").pending == 8);
	CHECK(QByteArray(parseAlertRequest("GET /heartbeat HTTP/1.1\r\n").status).startsWith("400"));

	ChzzkAlertServer server;
	CHECK(server.start(0) && server.port() != 0);
	QTcpSocket socket;
	socket.connectToHost(QHostAddress::LocalHost, server.port());
	QObject::connect(&socket, &QTcpSocket::errorOccurred, &app,
			 [&] { std::cerr << "Socket error: " << socket.errorString().toStdString() << '\n'; });
	until("connect", [&] { return socket.state() == QAbstractSocket::ConnectedState; });
	socket.write("GET /publish?type=follow&nickname=Split");
	socket.flush();
	QCoreApplication::processEvents();
	CHECK(published == 0);
	socket.write("%20Name HTTP/1.1\r\nHost: localhost\r\n\r\n");
	until("publish split request", [&] { return published == 1; });
	until("split request response", [&] { return socket.bytesAvailable() > 0; });
	CHECK(socket.readAll().startsWith("HTTP/1.1 204"));
	CHECK(received == "Split Name");
	auto transact = [&](const QByteArray &line) {
		QTcpSocket client;
		client.connectToHost(QHostAddress::LocalHost, server.port());
		until("client connect", [&] { return client.state() == QAbstractSocket::ConnectedState; });
		client.write(line + "\r\n\r\n");
		until("client response", [&] { return client.bytesAvailable() > 0; });
		return client.readAll();
	};
	CHECK(transact("GET /publish?type=follow&nickname=Same&eventId=feed%3A42 HTTP/1.1").startsWith("HTTP/1.1 204"));
	CHECK(published == 2);
	CHECK(transact("GET /publish?type=follow&nickname=Same&eventId=feed%3A42 HTTP/1.1").startsWith("HTTP/1.1 204"));
	CHECK(published == 2); // A lost acknowledgement may safely be retried.
	CHECK(transact("GET /heartbeat?session=test-bridge&pending=4 HTTP/1.1").startsWith("HTTP/1.1 204"));
	CHECK(transact("GET /status HTTP/1.1").contains("\"pending\":4"));
	CHECK(alertConnectionState().snapshot().bridges == 1);

	QTcpSocket oversized;
	QObject::connect(&oversized, &QTcpSocket::errorOccurred, &app,
			 [&] { std::cerr << "Oversized socket: " << oversized.errorString().toStdString() << '\n'; });
	oversized.connectToHost(QHostAddress::LocalHost, server.port());
	until("oversized connect", [&] { return oversized.state() == QAbstractSocket::ConnectedState; });
	oversized.write(QByteArray(17 * 1024, 'x'));
	until("oversized response", [&] { return oversized.bytesAvailable() > 0; });
	CHECK(oversized.readAll().startsWith("HTTP/1.1 414"));
	CHECK(published == 2);
	server.stop();
	CHECK(server.port() == 0);
	std::cout << "HTTP parsing and fragmented socket checks passed\n";
}
