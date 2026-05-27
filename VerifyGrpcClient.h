#pragma once
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "Const.h"
#include "Singleton.h"
#include <queue>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using message::GetVerifyReq;
using message::GetVerifyRes;
using message::VerifyService;

class RPConPool {
public:
	RPConPool(size_t poolsize, std::string host, std::string port);
	~RPConPool();
	void close();
	std::unique_ptr<VerifyService::Stub> getConnection();
	void releaseConnection(std::unique_ptr<VerifyService::Stub> con);

private:
	std::atomic<bool> m_stop;
	size_t m_poolsize;
	std::string m_host;
	std::string m_port;
	std::queue<std::unique_ptr<VerifyService::Stub>> m_connections;
	std::condition_variable m_cond;
	std::mutex m_mutex;
};

class VerifyGrpcClient : public Singleton<VerifyGrpcClient>
{
	friend class Singleton<VerifyGrpcClient>;

public:
	GetVerifyRes getVerifyCode(const std::string& email);

private:
	VerifyGrpcClient();

private:
	//std::unique_ptr<VerifyService::Stub> m_stub;
	std::unique_ptr<RPConPool> m_pool;
};

