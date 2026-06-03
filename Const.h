#pragma once
#include <functional>

enum ErrorCodes
{
	Success = 0,
	Error_Json = 1001,
	RPCFailed = 1002,
	VerifyExpired = 1003,
	VerifyCodeErr = 1004,
	UserExist = 1005,
	PasswordErr = 1006,
	EmailNotMatch = 1007,
	PasswdUpFailed = 1008,
	PasswdInvalid = 1009,
};

constexpr auto CODEPROFIX = "code_";

class Defer {
public:
	Defer(std::function<void()> func) : m_func(func) {}
	~Defer() {
		if (m_func) m_func();
	}

private:
	std::function<void()> m_func;
};
