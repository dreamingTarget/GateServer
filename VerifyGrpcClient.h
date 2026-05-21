#pragma once
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "Const.h"
#include "Singleton.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using message::GetVerifyReq;
using message::GetVerifyRes;
using message::VerifyService;

class VerifyGrpcClient : public Singleton<VerifyGrpcClient>
{
	friend class Singleton<VerifyGrpcClient>;

public:
	GetVerifyRes getVerifyCode(const std::string& email);

private:
	VerifyGrpcClient();

private:
	std::unique_ptr<VerifyService::Stub> m_stub;
};

