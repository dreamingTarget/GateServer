#include "MysqlDao.h"
#include "ConfigMgr.h"

MysqlPool::MysqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolsize)
	: m_url(url), m_user(user), m_pass(pass), m_schema(schema), m_poolsize(poolsize), m_b_stop(false)
{
	try {
		for (int i = 0; i < poolsize; i++) {
			sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
			auto* conn = driver->connect(url, user, pass);

			std::cout << "url: " << url << std::endl;

			conn->setSchema(schema);
			//获取当前时间戳
			auto currtime = std::chrono::system_clock::now().time_since_epoch();
			//将时间戳转换为秒
			long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currtime).count();
			m_pool.push(std::make_unique<SqlConnection>(conn, timestamp));
		}

		m_check_thread = std::thread([this]() {
			while (!m_b_stop) {
				checkConnection();
				std::this_thread::sleep_for(std::chrono::seconds(60));
			}
			});

		m_check_thread.detach();
	}
	catch (sql::SQLException& e) {
		// 处理异常
		std::cout << "mysql pool init failed, error is " << e.what() << std::endl;
	}
}

MysqlPool::~MysqlPool()
{
	std::lock_guard<std::mutex> locker(m_mtx);
	while (!m_pool.empty()) {
		m_pool.pop();
	}
}

void MysqlPool::checkConnection()
{
	std::lock_guard<std::mutex> locker(m_mtx);
	int poolsize = m_pool.size();
	//获取当前时间戳
	auto currtime = std::chrono::system_clock::now().time_since_epoch();
	//将时间戳转换为秒
	long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currtime).count();
	for (int i = 0; i < poolsize; i++) {
		auto conn = std::move(m_pool.front());
		m_pool.pop();
		Defer defer([this, &conn]() {
			m_pool.push(std::move(conn));
			});

		if (timestamp - conn->m_last_oper_time < 5) continue;

		try {
			std::unique_ptr<sql::Statement> state(conn->m_conn->createStatement());
			state->executeQuery("select 1");
			conn->m_last_oper_time = timestamp;
			//std::cout << "execute timer alive query , cur is " << timestamp << std::endl;
		}
		catch (sql::SQLException& e) {
			std::cout << "error keeping connection alive: " << e.what() << std::endl;
			//重新创建连接并替换旧的连接
			sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
			auto* newconn = driver->connect(m_url, m_user, m_pass);
			newconn->setSchema(m_schema);
			conn->m_conn.reset(newconn);
			conn->m_last_oper_time = timestamp;
		}
	}
}

std::unique_ptr<SqlConnection> MysqlPool::getConnection()
{
	std::unique_lock<std::mutex> locker(m_mtx);
	m_cond.wait(locker, [this]() {
		if (m_b_stop) return true;
		return !m_pool.empty();
		});

	if (m_b_stop) return nullptr;

	auto conn = std::move(m_pool.front());
	m_pool.pop();
	return conn;
}

void MysqlPool::retConnection(std::unique_ptr<SqlConnection> conn)
{
	std::unique_lock<std::mutex> locker(m_mtx);
	if (m_b_stop) return;
	m_pool.push(std::move(conn));
	m_cond.notify_one();
}

void MysqlPool::close() {
	m_b_stop = true;
	m_cond.notify_all();
}

MysqlDao::MysqlDao()
{
	auto& config = ConfigMgr::getInstance();
	const auto& host = config["Mysql"]["host"];
	const auto& port = config["Mysql"]["port"];
	const auto& pwd = config["Mysql"]["passwd"];
	const auto& schema = config["Mysql"]["schema"];
	const auto& user = config["Mysql"]["user"];

	std::cout << host << ", " << port << ", " << pwd << ", " << schema << ", " << user << std::endl;

	m_sqlpool = std::make_unique<MysqlPool>(host + ":" + port, user, pwd, schema, 5);
}

MysqlDao::~MysqlDao()
{
	m_sqlpool->close();
}

int MysqlDao::regUser(const std::string& name, const std::string& email, const std::string& pwd)
{
	auto conn = m_sqlpool->getConnection();

	// 优化：使用Defer，保证连接必归还，无需重复写retConnection
	Defer defer([this, &conn]() {
		m_sqlpool->retConnection(std::move(conn));
		});

	try {
		if (conn == nullptr) return -1;
		//准备调用存储过程
		std::unique_ptr<sql::PreparedStatement> state(conn->m_conn->prepareStatement("CALL reg_user(?, ?, ?, @result)"));
		//设置输入参数
		state->setString(1, name);
		state->setString(2, email);
		state->setString(3, pwd);
		//由于PreparedStatement不直接支持注册输出参数，我们需要使用会话变量或其他方法来获取输出参数的值
		//执行存储过程
		state->execute();
		//如果存储过程设置了会话变量或有其他方式获取输出参数的值，你可以在这里执行SELECT查询来获取它们
	    //例如，如果存储过程设置了一个会话变量@result来存储输出结果，可以这样获取：
		std::unique_ptr<sql::Statement> stateres(conn->m_conn->createStatement());
		std::unique_ptr<sql::ResultSet> res(stateres->executeQuery("select @result as result"));
		if (res->next()) {
			int result = res->getInt("result");
			std::cout << "result: " << result << std::endl;
			//m_sqlpool->retConnection(std::move(conn));
			return result;
		}
		//m_sqlpool->retConnection(std::move(conn));
		return -1;

	}
	catch (sql::SQLException& e) {
		//m_sqlpool->retConnection(std::move(conn));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return -1;
	}

	return 0;
}

bool MysqlDao::checkEmail(const std::string& name, const std::string& email) {
	auto conn = m_sqlpool->getConnection();
	Defer defer([this, &conn]() {m_sqlpool->retConnection(std::move(conn)); });
	try {
		if (conn == nullptr) return false;
		//准备查询语句
		std::unique_ptr<sql::PreparedStatement> state(conn->m_conn->prepareStatement("select email from user where name = ?"));
		//绑定参数
		state->setString(1, name);
		//执行查询
		std::unique_ptr<sql::ResultSet> res(state->executeQuery());
		//遍历结果集
		while (res->next()) {
			std::cout << "check email: " << res->getString("email") << std::endl;
			if (email != res->getString("email")) {
				return false;
			}
			return true;
		}
		return true;
	}
	catch (sql::SQLException& e) {
		//m_sqlpool->retConnection(std::move(conn));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::updatePwd(const std::string& name, const std::string& pwd) {
	auto conn = m_sqlpool->getConnection();
	Defer defer([this, &conn]() {m_sqlpool->retConnection(std::move(conn)); });
	try {
		if (conn == nullptr) return false;
		//准备查询语句
		std::unique_ptr<sql::PreparedStatement> state(conn->m_conn->prepareStatement("update user set pwd = ? where name = ?"));
		//绑定参数
		state->setString(1, pwd);
		state->setString(2, name);
		int updateCnt = state->executeUpdate();
		std::cout << "updateCnt: " << updateCnt << std::endl;
		return true;
	}
	catch (sql::SQLException& e) {
		//m_sqlpool->retConnection(std::move(conn));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}

}

bool MysqlDao::checkPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo)
{
	auto con = m_sqlpool->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		m_sqlpool->retConnection(std::move(con));
		});

	try {
		// 准备SQL语句
		std::unique_ptr<sql::PreparedStatement> pstmt(con->m_conn->prepareStatement("SELECT * FROM user WHERE email = ?"));
		pstmt->setString(1, email); // 将username替换为你要查询的用户名

		// 执行查询
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		std::string origin_pwd = "";
		// 遍历结果集
		while (res->next()) {
			origin_pwd = res->getString("pwd");
			// 输出查询到的密码
			std::cout << "Password: " << origin_pwd << std::endl;
			break;
		}

		if (pwd != origin_pwd) {
			return false;
		}
		userInfo.name = res->getString("name");
		userInfo.email = res->getString("email");
		userInfo.uid = res->getInt("uid");
		userInfo.pwd = origin_pwd;
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

