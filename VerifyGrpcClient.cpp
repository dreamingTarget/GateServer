#include "VerifyGrpcClient.h"

GetVerifyRes VerifyGrpcClient::getVerifyCode(const std::string& email)
{
	ClientContext context;
	GetVerifyReq request;
	GetVerifyRes response;
	request.set_email(email);
	Status status = m_stub->getVerifyCode(&context, request, &response);
	if (status.ok()) {
		return response;
		std::cout << response.error() << std::endl;
		std::cout << response.code() << std::endl;
	}
	else {
		response.set_error(ErrorCodes::RPCFailed);
		return response;
	}
}

VerifyGrpcClient::VerifyGrpcClient()
{
	std::shared_ptr<Channel> channel = grpc::CreateChannel("0.0.0.0:50051", grpc::InsecureChannelCredentials());
	m_stub = VerifyService::NewStub(channel);
}
