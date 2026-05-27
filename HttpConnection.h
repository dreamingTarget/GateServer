#pragma once
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <unordered_map>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class LogicSystem;

class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
	friend class LogicSystem;

public:
	HttpConnection(/*tcp::socket socket*/boost::asio::io_context& ioc);
	void start();
	inline tcp::socket& socket() { return m_socket; }

private:
	void checkDeadline();
	void writeResponse();
	void handleReq();
	void preParseGetParam();

private:
	tcp::socket m_socket;
	beast::flat_buffer m_buffer{8192};
	http::request<http::dynamic_body> m_req;
	http::response<http::dynamic_body> m_res;
	net::steady_timer m_deadline{ m_socket.get_executor(), std::chrono::seconds(60) };

	std::string m_get_url;
	std::unordered_map<std::string, std::string> m_get_params;
};

