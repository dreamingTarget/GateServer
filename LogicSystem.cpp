#include "LogicSystem.h"
#include "HttpConnection.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "Const.h"
#include "VerifyGrpcClient.h"
#include "RedisMgr.h"

bool LogicSystem::handleRequest(std::string url, std::shared_ptr<HttpConnection> conn)
{
	if (m_getHandlers.find(url) == m_getHandlers.end()) {
		return false;
	}
	m_getHandlers[url](conn);
	return true;
}

void LogicSystem::regGet(std::string url, HttpHandler handler)
{
	m_getHandlers[url] = handler;
}

void LogicSystem::regPost(std::string url, HttpHandler handler)
{
	m_postHandlers[url] = handler;
}

bool LogicSystem::handlePost(std::string url, std::shared_ptr<HttpConnection> conn)
{
	if (m_postHandlers.find(url) == m_postHandlers.end()) {
		return false;
	}
	m_postHandlers[url](conn);
	return true;
}

LogicSystem::LogicSystem() {
	regGet("/get_test", [](std::shared_ptr<HttpConnection> conn) {
		beast::ostream(conn->m_res.body()) << "receive get_test req";

		int i = 0;
		for (auto& param : conn->m_get_params) {
			beast::ostream(conn->m_res.body()) << "\nparam " << ++i << ": " << param.first << "=" << param.second;
		}
		});

	regPost("/get_verifycode", [](std::shared_ptr<HttpConnection> conn) {
		auto body_str = beast::buffers_to_string(conn->m_req.body().data());
		std::cout << "receive get_verifycode req, body: " << body_str << std::endl;
		conn->m_res.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parsingSuccessful = reader.parse(body_str, src_root);
		if (!parsingSuccessful) {
			std::cout << "Failed to parse JSON: " << reader.getFormatedErrorMessages() << std::endl;
			root["code"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(conn->m_res.body()) << jsonstr;
			return true;
		}

		if (!src_root.isMember("email")) {
			std::cout << "email field is missing in JSON" << std::endl;
			root["code"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(conn->m_res.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();

		GetVerifyRes res = VerifyGrpcClient::getInstance()->getVerifyCode(email);

		std::cout << "email: " << email << std::endl;
		std::cout << "grpc res code: " << res.code() << ", error: " << res.error() << std::endl;
		root["error"] = res.error();
		root["email"] = src_root["email"];
		std::string jsonstr = root.toStyledString();
		beast::ostream(conn->m_res.body()) << jsonstr;
		return true;
		});

	regPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->m_req.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->m_res.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->m_res.body()) << jsonstr;
			return true;
		}
		//先查找redis中email对应的验证码是否合理
		std::string  verify_code;
		bool b_get_verify = RedisMgr::getInstance()->get(CODEPROFIX + src_root["email"].asString(), verify_code);
		if (!b_get_verify) {
			std::cout << " get verify code expired" << std::endl;
			root["error"] = ErrorCodes::VerifyExpired;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->m_res.body()) << jsonstr;
			return true;
		}
		if (verify_code != src_root["verifycode"].asString()) {
			std::cout << " verify code error" << std::endl;
			root["error"] = ErrorCodes::VerifyCodeErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->m_res.body()) << jsonstr;
			return true;
		}

		//访问redis查找
		//bool b_usr_exist = RedisMgr::getInstance()->existsKey(src_root["user"].asString());
		//if (b_usr_exist) {
		//	std::cout << " user exist" << std::endl;
		//	root["error"] = ErrorCodes::UserExist;
		//	std::string jsonstr = root.toStyledString();
		//	beast::ostream(connection->m_res.body()) << jsonstr;
		//	return true;
		//}
		//查找数据库判断用户是否存在

		root["error"] = 0;
		root["email"] = src_root["email"];
		root["user"] = src_root["user"].asString();
		root["passwd"] = src_root["passwd"].asString();
		root["confirm"] = src_root["confirm"].asString();
		root["verifycode"] = src_root["verifycode"].asString();
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->m_res.body()) << jsonstr;
		return true;
		});
}
