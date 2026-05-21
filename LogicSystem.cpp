#include "LogicSystem.h"
#include "HttpConnection.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "Const.h"
#include "VerifyGrpcClient.h"

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
		root["error"] = res.error();
		root["email"] = src_root["email"];
		std::string jsonstr = root.toStyledString();
		beast::ostream(conn->m_res.body()) << jsonstr;
		return true;
		});
}
