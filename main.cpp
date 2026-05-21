#include <iostream>
#include "CServer.h"
#include "ConfigMgr.h"

int main()
{
	try {
		ConfigMgr config;
		std::string port_str = config["GateServer"]["port"];
		unsigned short port = static_cast<unsigned short>(std::stoi(port_str));

		//unsigned short port = 8080;
		net::io_context ioc{ 1 };
		boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
		signals.async_wait([&](beast::error_code const& error, int) {
			if (error) return;
			ioc.stop();
			});
		std::make_shared<CServer>(ioc, port)->start();
		std::cout << "server is running on port " << port << std::endl;
		ioc.run();
	}
	catch (const std::exception& exp)
	{
		std::cout << "server error: " << exp.what() << std::endl;
		return EXIT_FAILURE;
	}
}
