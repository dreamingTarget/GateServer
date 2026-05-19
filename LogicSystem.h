#pragma once
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include "Singleton.h"
#include <functional>
#include <map>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class HttpConnection;
using HttpHandler = std::function<void(std::shared_ptr<HttpConnection> conn)>;

class LogicSystem : public Singleton<LogicSystem>,
	public std::enable_shared_from_this<LogicSystem>
{
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem() = default;
	bool handleRequest(std::string url, std::shared_ptr<HttpConnection> conn);
	void regGet(std::string url, HttpHandler handler);
	void regPost(std::string url, HttpHandler handler);
	bool handlePost(std::string url, std::shared_ptr<HttpConnection> conn);

private:
	LogicSystem();
	std::map<std::string, HttpHandler> m_getHandlers;
	std::map<std::string, HttpHandler> m_postHandlers;
};

