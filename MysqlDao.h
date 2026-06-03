#pragma once
#include "Const.h"
#include <thread>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/exception.h>
#include <queue>
#include <mutex>

class SqlConnection {
	friend class MysqlPool;
	friend class MysqlDao;

public:
	SqlConnection(sql::Connection* conn, int64_t lasttime);

private:
	std::unique_ptr<sql::Connection> m_conn;
	int64_t m_last_oper_time;

};

class MysqlPool {
public:
	MysqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolsize);
	~MysqlPool();

	void checkConnection();
	std::unique_ptr<SqlConnection> getConnection();
	void retConnection(std::unique_ptr<SqlConnection> conn);
	void close();

private:
	std::string m_url;
	std::string m_user;
	std::string m_pass;
	std::string m_schema;
	int m_poolsize;
	std::queue<std::unique_ptr<SqlConnection>> m_pool;
	std::mutex m_mtx;
	std::condition_variable m_cond;
	std::atomic<bool> m_b_stop;
	std::thread m_check_thread;

};

struct UserInfo {
	std::string name;
	std::string pwd;
	int uid;
	std::string email;
};

class MysqlDao {
public:
	MysqlDao();
	~MysqlDao();
	int regUser(const std::string& name, const std::string& email, const std::string& pwd);

	//bool checkEmail(const std::string& name, const std::string& email);
	//bool updatePwd(const std::string& name, const std::string& pwd);
	//bool checkPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo);

private:
	std::unique_ptr<MysqlPool> m_sqlpool;
};
