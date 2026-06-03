#include "MysqlDao.h"
#include "Const.h"
#include "ConfigMgr.h"

SqlConnection::SqlConnection(sql::Connection* conn, int64_t lasttime) 
	: m_conn(conn), m_last_oper_time(lasttime) {

}

MysqlPool::MysqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolsize)
	: m_url(url), m_user(user), m_pass(pass), m_schema(schema), m_poolsize(poolsize), m_b_stop(false)
{
	try {
		for (int i = 0; i < m_poolsize; ++i) {
			sql::Connection* conn = sql::mysql::get_mysql_driver_instance()->connect(m_url, m_user, m_pass);
			conn->setSchema(m_schema);
			auto current_time = std::chrono::system_clock::now().time_since_epoch();
			auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(current_time).count();
			//std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			m_pool.emplace(std::make_unique<SqlConnection>(conn, timestamp));
		}

		m_check_thread = std::thread([this]() {
			while (!m_b_stop) {
				checkConnection();
				std::this_thread::sleep_for(std::chrono::seconds(60));
			}
			});

		m_check_thread.detach();

	} catch (sql::SQLException &e) {
		std::cerr << "SQLException: " << e.what() << std::endl;
		std::cerr << "MySQL error code: " << e.getErrorCode() << std::endl;
		std::cerr << "SQLState: " << e.getSQLState() << std::endl;
	}
}

MysqlPool::~MysqlPool()
{
	std::lock_guard<std::mutex> lock(m_mtx);
	while (!m_pool.empty()) {
		auto& conn = m_pool.front();
		conn->m_conn->close();
		m_pool.pop();
	}
}

void MysqlPool::checkConnection()
{
	std::lock_guard<std::mutex> lock(m_mtx);
	int poolsize = m_pool.size();
	auto current_time = std::chrono::system_clock::now().time_since_epoch();
	auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(current_time).count();
	for (int i = 0; i < poolsize; ++i) {
		auto conn = std::move(m_pool.front());
		m_pool.pop();
		Defer defer([this, &conn]() {
			//std::lock_guard<std::mutex> lock(m_mtx);
			m_pool.emplace(std::move(conn));
			});
		if (timestamp - conn->m_last_oper_time < 5) continue;

		try {
			std::unique_ptr<sql::Statement> stmt(conn->m_conn->createStatement());
			stmt->execute("SELECT 1");
			conn->m_last_oper_time = timestamp;
			std::cout << "Connection is healthy. cur is " << timestamp << std::endl;

		}
		catch (sql::SQLException& e) {
			std::cerr << "SQLException: " << e.what() << std::endl;
			std::cerr << "MySQL error code: " << e.getErrorCode() << std::endl;
			std::cerr << "SQLState: " << e.getSQLState() << std::endl;

			//重新创建连接并替换旧的连接
			sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
			auto* new_conn = driver->connect(m_url, m_user, m_pass);
			new_conn->setSchema(m_schema);
			conn->m_conn.reset(new_conn);
			conn->m_last_oper_time = timestamp;
		}
	}
}

std::unique_ptr<SqlConnection> MysqlPool::getConnection()
{
	std::unique_lock<std::mutex> lock(m_mtx);
	m_cond.wait(lock, [this]() {
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
	std::lock_guard<std::mutex> lock(m_mtx);
	if (m_b_stop) return;
	m_pool.emplace(std::move(conn));
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
	const auto& user = config["Mysql"]["user"];
	const auto& pass = config["Mysql"]["passwd"];
	const auto& schema = config["Mysql"]["schema"];
	m_sqlpool = std::make_unique<MysqlPool>(host + ":" + port, user, pass, schema, 5);
}

MysqlDao::~MysqlDao()
{
	m_sqlpool->close();
}

int MysqlDao::regUser(const std::string& name, const std::string& email, const std::string& pwd)
{
	auto conn = m_sqlpool->getConnection();
	try
	{
		if (conn == nullptr) return false;
		std::unique_ptr<sql::PreparedStatement> stmt(conn->m_conn->prepareStatement("CALL reg_user(?,?,?,@result)"));
		stmt->setString(1, name);
		stmt->setString(2, email);
		stmt->setString(3, pwd);

		stmt->execute();

		std::unique_ptr<sql::Statement> res_stmt(conn->m_conn->createStatement());
		std::unique_ptr<sql::ResultSet> res(res_stmt->executeQuery("SELECT @result AS result"));
		if (res->next()) {
			int result = res->getInt("result");
			std::cout << "Registration result: " << result << std::endl;
			m_sqlpool->retConnection(std::move(conn));
			return result;
		}
		m_sqlpool->retConnection(std::move(conn));
		return -1;
	}
	catch (const sql::SQLException& e)
	{
		m_sqlpool->retConnection(std::move(conn));
		std::cerr << "SQLException: " << e.what();
		std::cerr << "(MYSQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return -1;
	}
	return 0;
}

