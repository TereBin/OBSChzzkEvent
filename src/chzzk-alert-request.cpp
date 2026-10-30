#include "chzzk-alert-request.hpp"
#include "chzzk-alert-protocol.hpp"

#include <QUrl>
#include <QUrlQuery>
#include <algorithm>

AlertRequest parseAlertRequest(const QByteArray &requestLine)
{
	const auto parts = requestLine.trimmed().split(' ');
	if (parts.size() != 3 || !parts[2].startsWith("HTTP/1."))
		return {};
	// URLSearchParams uses form encoding: '+' is a space, while '%2B' is a literal plus.
	QByteArray target = parts[1];
	const auto queryStart = target.indexOf('?');
	if (queryStart >= 0)
		target = target.left(queryStart + 1) + target.mid(queryStart + 1).replace("+", "%20");
	const QUrl url = QUrl::fromEncoded(target);
	const auto path = url.path();
	if (parts[0] == "OPTIONS" && (path == "/publish" || path == "/heartbeat" || path == "/status")) {
		AlertRequest result{"204 No Content", true, {}};
		result.kind = AlertRequest::Kind::Preflight;
		return result;
	}
	if (parts[0] == "GET" && path == "/status") {
		AlertRequest result{"200 OK", true, {}};
		result.kind = AlertRequest::Kind::Status;
		return result;
	}
	const QUrlQuery query(url);
	if (parts[0] == "GET" && path == "/heartbeat") {
		AlertRequest result{"204 No Content", true, {}};
		result.session = query.queryItemValue("session");
		if (result.session.isEmpty() || result.session.size() > AlertProtocol::maxSessionLength)
			return {"400 Bad Request", true, {}};
		result.kind = AlertRequest::Kind::Heartbeat;
		result.pending = std::clamp(query.queryItemValue("pending").toInt(), 0, AlertProtocol::maxPending);
		const auto error = query.queryItemValue("error");
		if (error == "connection" || error == "http" || error == "expired" || error == "full")
			result.error = error;
		return result;
	}
	if (parts[0] != "GET" || url.path() != QStringLiteral("/publish"))
		return {"404 Not Found", false, {}};
	const auto nickname = query.queryItemValue(QStringLiteral("nickname"), QUrl::FullyDecoded).trimmed();
	const auto type = query.queryItemValue(QStringLiteral("type"));
	if ((type != QStringLiteral("follow") && type != QStringLiteral("test")) || nickname.isEmpty())
		return {"400 Bad Request", true, {}};
	if (type == QStringLiteral("test") && nickname.size() > AlertProtocol::maxTestNicknameLength)
		return {"400 Bad Request", true, {}};
	AlertRequest result{"204 No Content", true, nickname};
	result.kind = type == QStringLiteral("test") ? AlertRequest::Kind::Test : AlertRequest::Kind::Follow;
	result.eventId = query.queryItemValue("eventId", QUrl::FullyDecoded);
	if (result.eventId.size() > AlertProtocol::maxEventIdLength)
		return {"400 Bad Request", true, {}};
	return result;
}

QByteArray alertResponse(const AlertRequest &request)
{
	QByteArray response = "HTTP/1.1 ";
	response += request.status;
	response += "\r\n";
	if (request.cors)
		response += "Access-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET, OPTIONS\r\n"
			    "Access-Control-Allow-Headers: Content-Type, Cache-Control, Pragma\r\n"
			    "Access-Control-Allow-Private-Network: true\r\n";
	if (!request.body.isEmpty())
		response += "Content-Type: application/json; charset=utf-8\r\n";
	response += "Content-Length: " + QByteArray::number(request.body.size()) + "\r\nConnection: close\r\n\r\n";
	response += request.body;
	return response;
}
