#include "HttpConnection.h"
#include <iostream>
#include "LogicSystem.h"

HttpConnection::HttpConnection(tcp::socket socket) : m_socket(std::move(socket))
{
}

void HttpConnection::start() {
	auto self = shared_from_this();
	http::async_read(m_socket, m_buffer, m_req, [self](beast::error_code ec, std::size_t bytes_transferred) {
		try
		{
			if (ec) {
				std::cout << "read request error: " << ec.message() << std::endl;
				return;
			}
			boost::ignore_unused(bytes_transferred);
			self->handleReq();
			self->checkDeadline();
		}
		catch (const std::exception& exp)
		{
			std::cout << "handle request error: " << exp.what() << std::endl;
		}
		});
}

void HttpConnection::checkDeadline()
{
	auto self = shared_from_this();
	m_deadline.async_wait([self](beast::error_code ec) {
		if (!ec) {
			self->m_socket.close(ec);
		}
		});
}

void HttpConnection::writeResponse()
{
	auto self = shared_from_this();
	m_res.content_length(m_res.body().size());
	http::async_write(m_socket, m_res, [self](beast::error_code ec, std::size_t bytes_transferred) {
		self->m_socket.shutdown(tcp::socket::shutdown_send, ec);
		self->m_deadline.cancel();
		});
}

void HttpConnection::handleReq()
{
	m_res.version(m_req.version());
	m_res.keep_alive(false);
	if (m_req.method() == http::verb::get)
	{
		bool success = LogicSystem::getInstance()->handleRequest(m_req.target(), shared_from_this());
		if (!success) {
			m_res.result(http::status::not_found);
			m_res.set(http::field::content_type, "text/html");
			beast::ostream(m_res.body()) << "url not found\r\n";
			writeResponse();
			return;
		}
		m_res.result(http::status::ok);
		m_res.set(http::field::server, "GateServer");
		writeResponse();
	}
}

