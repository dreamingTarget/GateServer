#include "MysqlMgr.h"

MysqlMgr::~MysqlMgr() {

}

int MysqlMgr::regUser(const std::string& name, const std::string& email, const std::string& pwd) {
	return m_dao.regUser(name, email, pwd);
}

MysqlMgr::MysqlMgr() {

}

bool MysqlMgr::checkEmail(const std::string& name, const std::string& email) {
	return m_dao.checkEmail(name, email);
}

bool MysqlMgr::updatePwd(const std::string& name, const std::string& pwd) {
	return m_dao.updatePwd(name, pwd);
}

bool MysqlMgr::checkPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo) {
	return m_dao.checkPwd(email, pwd, userInfo);
}
