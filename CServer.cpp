#include "CServer.h"
#include "HttpConnection.h"
#include <iostream>
#include "AsioIOServicePool.h"

CServer::CServer(net::io_context& ioc, unsigned short& port) :
	m_acceptor(ioc, tcp::endpoint(tcp::v4(), port)),
	m_ioc(ioc)
	//m_socket(ioc)
{

}

void CServer::start()
{
	auto self = shared_from_this();

	auto& ioc = AsioIOServicePool::getInstance()->getIOService();
	std::shared_ptr<HttpConnection> conn = std::make_shared<HttpConnection>(ioc);

	m_acceptor.async_accept(conn->socket(), [self, conn](beast::error_code ec) {
		try
		{
			if (ec) {
				self->start();
				return;
			}
			//std::make_shared<HttpConnection>(std::move(self->m_socket))->start();
			conn->start();
			self->start();
		}
		catch (const std::exception& exp)
		{
			std::cout << "accept connection error: " << exp.what() << std::endl;
		}
		});
}

