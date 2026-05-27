#include "HttpConnection.h"
#include <iostream>
#include "LogicSystem.h"

HttpConnection::HttpConnection(/*tcp::socket socket*/boost::asio::io_context& ioc) 
	: m_socket(/*std::move(socket)*/ioc)
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
	//设置utf8返回
	m_res.set(http::field::content_type, "text/html; charset=utf-8");
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
		preParseGetParam();
		bool success = LogicSystem::getInstance()->handleRequest(m_get_url, shared_from_this());
		if (!success) {
			m_res.result(http::status::not_found);
			//m_res.set(http::field::content_type, "text/html");
			beast::ostream(m_res.body()) << "url not found\r\n";
			writeResponse();
			return;
		}
		m_res.result(http::status::ok);
		m_res.set(http::field::server, "GateServer");
		writeResponse();
	}

	if (m_req.method() == http::verb::post)
	{
		bool success = LogicSystem::getInstance()->handlePost(m_req.target(), shared_from_this());
		if (!success) {
			m_res.result(http::status::not_found);
			beast::ostream(m_res.body()) << "url not found\r\n";
			writeResponse();
			return;
		}
		m_res.result(http::status::ok);
		m_res.set(http::field::server, "GateServer");
		writeResponse();
	}
}

unsigned char toHex(unsigned char c)
{
	return c > 9 ? c + 55 : c + 48;
}

unsigned char fromHex(unsigned char c)
{
	//改成c <= E/e更好，0~15
	if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
	if (c >= 'a' && c <= 'z') return c - 'a' + 10;
	if (c >= '0' && c <= '9') return c - '0';
	return 0;
}

std::string urlEncode(const std::string& str)
{
	std::string result;
	for (size_t i = 0; i < str.size(); ++i)
	{
		char c = str[i];
		if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
		{
			result += c;
		}
		else if (c == ' ')
		{
			result += '+';
		}
		else
		{
			result += '%';
			result += toHex((c >> 4) & 0xF);
			result += toHex(c & 0xF);
		}
	}
	return result;
}

std::string urlDecode(const std::string& str)
{
	std::string result;
	//key=111&key2=%E4%BD%A0%E5%A5%BD
	for (size_t i = 0; i < str.size(); ++i)
	{
		char c = str[i];
		if (c == '+')
		{
			result += ' ';
		}
		else if (c == '%' && i + 2 < str.size())
		{
			unsigned char high = fromHex(str[i + 1]);
			unsigned char low = fromHex(str[i + 2]);
			result += (high << 4) | low;
			i += 2;
		}
		else
		{
			result += c;
		}
	}
	return result;
}

void HttpConnection::preParseGetParam() {
	// 提取 URI  http://localhost/ get_test?key1=val1&key2=val2
	// http://localhost:8080/get_test?key=111&key2=你好
	//key = 111 & key2 = % E4 % BD % A0 % E5 % A5 % BD
	auto uri = m_req.target();
	// 查找查询字符串的开始位置（即 '?' 的位置）  
	auto query_pos = uri.find('?');
	if (query_pos == std::string::npos) {
		m_get_url = uri;
		return;
	}

	m_get_url = uri.substr(0, query_pos);
	std::string query_string = uri.substr(query_pos + 1);
	std::string key;
	std::string value;
	size_t pos = 0;
	while ((pos = query_string.find('&')) != std::string::npos) {
		auto pair = query_string.substr(0, pos);
		size_t eq_pos = pair.find('=');
		if (eq_pos != std::string::npos) {
			key = urlDecode(pair.substr(0, eq_pos)); // 假设有 url_decode 函数来处理URL解码  
			value = urlDecode(pair.substr(eq_pos + 1));
			m_get_params[key] = value;
		}
		query_string.erase(0, pos + 1);
	}
	// 处理最后一个参数对（如果没有 & 分隔符）  
	if (!query_string.empty()) {
		size_t eq_pos = query_string.find('=');
		if (eq_pos != std::string::npos) {
			key = urlDecode(query_string.substr(0, eq_pos));
			value = urlDecode(query_string.substr(eq_pos + 1));
			m_get_params[key] = value;
		}
	}
}

