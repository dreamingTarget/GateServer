#include "VerifyGrpcClient.h"
#include "ConfigMgr.h"

GetVerifyRes VerifyGrpcClient::getVerifyCode(const std::string& email)
{
	ClientContext context;
	GetVerifyReq request;
	GetVerifyRes response;
	request.set_email(email);
	auto stub = m_pool->getConnection();
	Status status = stub->getVerifyCode(&context, request, &response);
	if (status.ok()) {
		m_pool->releaseConnection(std::move(stub));
		return response;
		std::cout << response.error() << std::endl;
		std::cout << response.code() << std::endl;
	}
	else {
		m_pool->releaseConnection(std::move(stub));
		response.set_error(ErrorCodes::RPCFailed);
		return response;
	}
}

VerifyGrpcClient::VerifyGrpcClient()
{
	//std::shared_ptr<Channel> channel = grpc::CreateChannel("127.0.0.1:50051", grpc::InsecureChannelCredentials());
	//m_stub = VerifyService::NewStub(channel);
	auto& config = ConfigMgr::getInstance();
	std::string host = config["VerifyServer"]["host"];
	std::string port = config["VerifyServer"]["port"];
	m_pool.reset(new RPConPool(5, host, port));
}

RPConPool::RPConPool(size_t poolsize, std::string host, std::string port)
	: m_stop(false), m_poolsize(poolsize), m_host(host), m_port(port) {
	for (size_t i = 0; i < m_poolsize; ++i) {
		auto channel = grpc::CreateChannel(m_host + ":" + m_port, grpc::InsecureChannelCredentials());
		m_connections.push(VerifyService::NewStub(channel));
	}
}

RPConPool::~RPConPool()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	while (!m_connections.empty())
	{
		m_connections.pop();
	}
}

void RPConPool::close() {
	m_stop = true;
	m_cond.notify_all();
}

std::unique_ptr<VerifyService::Stub> RPConPool::getConnection()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	m_cond.wait(lock, [this]() /*{ return !m_connections.empty() || m_stop; });*/ {
		if (m_stop) return true;
		return !m_connections.empty();
		});
	//if (m_stop && m_connections.empty()) {
	//	return nullptr;
	//}
	if (m_stop) {
		return nullptr;
	}
	auto connection = std::move(m_connections.front());
	m_connections.pop();
	return connection;
}

void RPConPool::releaseConnection(std::unique_ptr<VerifyService::Stub> con)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if (!m_stop) {
		m_connections.push(std::move(con));
		m_cond.notify_one();
	}

}
