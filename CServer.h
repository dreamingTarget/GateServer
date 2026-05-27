#pragma once
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class CServer : public std::enable_shared_from_this<CServer>
{
public:
	CServer(net::io_context& ioc, unsigned short& port);
	void start();

private:
	tcp::acceptor m_acceptor;
	net::io_context& m_ioc;
	//tcp::socket m_socket;
};

