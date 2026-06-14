#include "StatusGrpcClient.h"

StatusConPool::StatusConPool(size_t poolSize, std::string host, std::string port)
	: m_poolsize(poolSize), m_host(host), m_port(port), m_stop(false) {
	for (size_t i = 0; i < m_poolsize; ++i) {

		std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port,
			grpc::InsecureChannelCredentials());

		m_connections.push(StatusService::NewStub(channel));
	}
}

StatusConPool::~StatusConPool() {
	std::lock_guard<std::mutex> lock(m_mutex);
	close();
	while (!m_connections.empty()) {
		m_connections.pop();
	}
}

std::unique_ptr<StatusService::Stub> StatusConPool::getConnection()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	m_cond.wait(lock, [this] {
		if (m_stop) {
			return true;
		}
		return !m_connections.empty();
		});
	//如果停止则直接返回空指针
	if (m_stop) {
		return  nullptr;
	}
	auto context = std::move(m_connections.front());
	m_connections.pop();
	return context;
}

void StatusConPool::returnConnection(std::unique_ptr<StatusService::Stub> context)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_stop) {
		return;
	}
	m_connections.push(std::move(context));
	m_cond.notify_one();
}

void StatusConPool::close()
{
	m_stop = true;
	m_cond.notify_all();
}

StatusGrpcClient::~StatusGrpcClient()
{

}

GetChatServerRes StatusGrpcClient::getChatServer(int uid)
{
	ClientContext context;
	GetChatServerRes reply;
	GetChatServerReq request;
	request.set_uid(uid);
	auto stub = m_pool->getConnection();
	Status status = stub->getChatServer(&context, request, &reply);
	Defer defer([&stub, this]() {
		m_pool->returnConnection(std::move(stub));
		});
	if (status.ok()) {
		return reply;
	}
	else {
		reply.set_error(ErrorCodes::RPCFailed);
		return reply;
	}
}

LoginRes StatusGrpcClient::login(int uid, std::string token)
{
	return LoginRes();
}

StatusGrpcClient::StatusGrpcClient()
{
	auto& gCfgMgr = ConfigMgr::getInstance();
	std::string host = gCfgMgr["StatusServer"]["host"];
	std::string port = gCfgMgr["StatusServer"]["port"];
	m_pool.reset(new StatusConPool(5, host, port));
}
