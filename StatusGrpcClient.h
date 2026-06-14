#pragma once
#include "Const.h"
#include "Singleton.h"
#include "ConfigMgr.h"
#include <grpcpp/grpcpp.h> 
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <atomic>
#include <queue>

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetChatServerReq;
using message::GetChatServerRes;
using message::LoginRes;
using message::LoginReq;
using message::StatusService;

class StatusConPool {
public:
	StatusConPool(size_t poolSize, std::string host, std::string port);
	~StatusConPool();
	std::unique_ptr<StatusService::Stub> getConnection();
	void returnConnection(std::unique_ptr<StatusService::Stub> context);
	void close();

private:
	std::atomic<bool> m_stop;
	size_t m_poolsize;
	std::string m_host;
	std::string m_port;
	std::queue<std::unique_ptr<StatusService::Stub>> m_connections;
	std::mutex m_mutex;
	std::condition_variable m_cond;
};

class StatusGrpcClient :public Singleton<StatusGrpcClient>
{
	friend class Singleton<StatusGrpcClient>;
public:
	~StatusGrpcClient();
	GetChatServerRes getChatServer(int uid);
	LoginRes login(int uid, std::string token);
private:
	StatusGrpcClient();
	std::unique_ptr<StatusConPool> m_pool;

};