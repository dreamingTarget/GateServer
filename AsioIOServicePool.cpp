#include "AsioIOServicePool.h"
#include <iostream>

AsioIOServicePool::~AsioIOServicePool()
{
	stop();
	std::cout << "AsioIOServicePool destructor called" << std::endl;
}

boost::asio::io_context& AsioIOServicePool::getIOService()
{
	// TODO: 在此处插入 return 语句
	auto& service = m_ioServices[m_nextIOService++];
	if (m_nextIOService == m_ioServices.size()) {
		m_nextIOService = 0;
	}
	return service;
}

void AsioIOServicePool::stop() {
	for (auto& work : m_works) {
		work->get_io_context().stop();
		work.reset();
	}
	for (auto& thread : m_threads) {
		if (thread.joinable()) {
			thread.join();
		}
	}
}

AsioIOServicePool::AsioIOServicePool(std::size_t size) : 
	m_nextIOService(0), m_ioServices(size), m_works(size)
{
	/*vector 不能直接存 Work，因为 Work 不能拷贝移动
	普通局部对象会自动销毁，指针会变成野指针
	必须 new / make_unique 创建堆对象，让指针长期安全指向它*/
	for (std::size_t i = 0; i < size; ++i) {
		m_works[i] = std::make_unique<Work>(m_ioServices[i]);
	}
	for (std::size_t i = 0; i < size; ++i) {
		m_threads.emplace_back([this, i]() {
			m_ioServices[i].run();
			});
	}
}
