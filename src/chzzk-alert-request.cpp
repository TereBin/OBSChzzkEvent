#include "chzzk-alert-request.hpp"

#include <QUrl>
#include <QUrlQuery>

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
	if (parts[0] != "GET" || url.path() != QStringLiteral("/publish"))
		return {"404 Not Found", false, {}};
	const QUrlQuery query(url);
	const auto nickname = query.queryItemValue(QStringLiteral("nickname"), QUrl::FullyDecoded).trimmed();
	if (query.queryItemValue(QStringLiteral("type")) != QStringLiteral("follow") || nickname.isEmpty())
		return {"400 Bad Request", true, {}};
	return {"204 No Content", true, nickname};
}

QByteArray alertResponse(const AlertRequest &request)
{
	QByteArray response = "HTTP/1.1 ";
	response += request.status;
	response += "\r\n";
	if (request.cors)
		response += "Access-Control-Allow-Origin: *\r\n";
	response += "Content-Length: 0\r\nConnection: close\r\n\r\n";
	return response;
}
