#pragma once
#include "MysqlDao.h"
#include "Singleton.h"

class MysqlMgr : public Singleton<MysqlMgr>
{
	friend class Singleton<MysqlMgr>;
public:
	~MysqlMgr();
	int regUser(const std::string& name, const std::string& email, const std::string& pwd);

private:
	MysqlMgr();

private:
	MysqlDao m_dao;

};

