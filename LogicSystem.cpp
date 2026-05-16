#include "LogicSystem.h"
#include "HttpConnection.h"

bool LogicSystem::handleRequest(std::string url, std::shared_ptr<HttpConnection> conn)
{
	if (m_getHandlers.find(url) == m_getHandlers.end()) {
		return false;
	}
	m_getHandlers[url](conn);
	return true;
}

void LogicSystem::regGet(std::string url, HttpHandler handler)
{
	m_getHandlers[url] = handler;
}

LogicSystem::LogicSystem() {
	regGet("/get_test", [](std::shared_ptr<HttpConnection> conn) {
		beast::ostream(conn->m_res.body()) << "receive get_test req";
		});
}
