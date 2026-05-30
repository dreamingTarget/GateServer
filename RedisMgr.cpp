#include "RedisMgr.h"
#include "ConfigMgr.h"

RedisMgr::~RedisMgr()
{
	close();
}

//bool RedisMgr::connect(const std::string& ip, int port)
//{
//	conn = redisConnect(ip.c_str(), port);
//    if (conn == nullptr || conn->err) {
//        if (conn) {
//            printf("Error: %s\n", conn->errstr);
//            redisFree(conn);
//            conn = nullptr;
//        }
//        else {
//            printf("Can't allocate redis context\n");
//        }
//        return false;
//	}
//    return true;
//}

bool RedisMgr::get(const std::string& key, std::string& value)
{
	auto conn = m_redisConnPool->getConnection();
    if (conn == nullptr) {
        printf("Error: no available Redis connection\n");
        return false;
	}
	auto reply = (redisReply*)redisCommand(conn, "GET %s", key.c_str());
    if (reply == nullptr) {
        printf("Error: %s\n", conn->errstr);
		freeReplyObject(reply);
		m_redisConnPool->releaseConnection(conn);
        return false;
	}
    if (reply->type != REDIS_REPLY_STRING) {
		std::cout << "Error: reply type is not string" << std::endl;
		freeReplyObject(reply);
		m_redisConnPool->releaseConnection(conn);
		return false;
	}
    value = reply->str;
    freeReplyObject(reply);
	std::cout << "GET " << key << " = " << value << std::endl;
    m_redisConnPool->releaseConnection(conn);
    return true;
}

bool RedisMgr::set(const std::string& key, const std::string& value)
{
    auto conn = m_redisConnPool->getConnection();
    if (conn == nullptr) {
        printf("Error: no available Redis connection\n");
        return false;
    }
	auto reply = (redisReply*)redisCommand(conn, "SET %s %s", key.c_str(), value.c_str());
    if (reply == nullptr) {
        printf("Error: %s\n", conn->errstr);
        m_redisConnPool->releaseConnection(conn);
		freeReplyObject(reply);
		return false;
    }
    if (reply->type != REDIS_REPLY_STATUS || std::string(reply->str) != "OK") {
        std::cout << "Error: SET command failed" << std::endl;
        m_redisConnPool->releaseConnection(conn);
		freeReplyObject(reply);
		return false;
    }
	freeReplyObject(reply);
    m_redisConnPool->releaseConnection(conn);
	std::cout << "SET " << key << " = " << value << std::endl;
    return true;
}

bool RedisMgr::auth(const std::string& password)
{
    auto conn = m_redisConnPool->getConnection();
    if (conn == nullptr) {
        printf("Error: no available Redis connection\n");
        return false;
    }
    auto reply = (redisReply*)redisCommand(conn, "AUTH %s", password.c_str());
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        printf("Error: %s\n", conn->errstr);
		freeReplyObject(reply);
        m_redisConnPool->releaseConnection(conn);
		return false;
    }
	freeReplyObject(reply);
    m_redisConnPool->releaseConnection(conn);
	std::cout << "AUTH success" << std::endl;
    return true;
}

bool RedisMgr::lpush(const std::string& key, const std::string& value)
{
    auto conn = m_redisConnPool->getConnection();
    if (conn == nullptr) {
        printf("Error: no available Redis connection\n");
        return false;
    }
    auto reply = (redisReply*)redisCommand(conn, "LPUSH %s %s", key.c_str(), value.c_str());
    if (reply == nullptr) {
        printf("Error: %s\n", conn->errstr);
        freeReplyObject(reply);
        m_redisConnPool->releaseConnection(conn);
        return false;
    }
    if (reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0) {
        std::cout << "Error: LPUSH command failed" << std::endl;
        freeReplyObject(reply);
        m_redisConnPool->releaseConnection(conn);
        return false;
	}
	std::cout << "LPUSH " << key << " " << value << ", list length is now " << reply->integer << std::endl;
	freeReplyObject(reply);
    m_redisConnPool->releaseConnection(conn);
    return true;
}

bool RedisMgr::lpop(const std::string& key, std::string& value)
{
    auto conn = m_redisConnPool->getConnection();
    if (conn == nullptr) {
        printf("Error: no available Redis connection\n");
        return false;
    }
    auto reply = (redisReply*)redisCommand(conn, "LPOP %s", key.c_str());
    if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
        printf("Error: %s\n", conn->errstr);
        freeReplyObject(reply);
        m_redisConnPool->releaseConnection(conn);
        return false;
	}
	value = reply->str;
	std::cout << "LPOP " << key << " = " << value << std::endl;
	freeReplyObject(reply);
    m_redisConnPool->releaseConnection(conn);
    return true;
}

bool RedisMgr::rpush(const std::string& key, const std::string& value)
{
    auto conn = m_redisConnPool->getConnection();
    if (conn == nullptr) {
        printf("Error: no available Redis connection\n");
        return false;
    }
    auto reply = (redisReply*)redisCommand(conn, "RPUSH %s %s", key.c_str(), value.c_str());
    if (reply == nullptr) {
        printf("Error: %s\n", conn->errstr);
        freeReplyObject(reply);
        m_redisConnPool->releaseConnection(conn);
        return false;
    }
    if (reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0) {
        std::cout << "Error: RPUSH command failed" << std::endl;
        freeReplyObject(reply);
        m_redisConnPool->releaseConnection(conn);
        return false;
    }
    std::cout << "RPUSH " << key << " " << value << ", list length is now " << reply->integer << std::endl;
    freeReplyObject(reply);
    m_redisConnPool->releaseConnection(conn);
    return true;
}

bool RedisMgr::rpop(const std::string& key, std::string& value)
{
    auto conn = m_redisConnPool->getConnection();
    if (conn == nullptr) {
        printf("Error: no available Redis connection\n");
        return false;
    }
    auto reply = (redisReply*)redisCommand(conn, "RPOP %s", key.c_str());
    if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
        printf("Error: %s\n", conn->errstr);
        freeReplyObject(reply);
        m_redisConnPool->releaseConnection(conn);
        return false;
    }
    value = reply->str;
    std::cout << "RPOP " << key << " = " << value << std::endl;
    freeReplyObject(reply);
    m_redisConnPool->releaseConnection(conn);
    return true;
}

bool RedisMgr::hset(const std::string& key, const std::string& field, const std::string& value)
{
    auto conn = m_redisConnPool->getConnection();
    if (conn == nullptr) {
        printf("Error: no available Redis connection\n");
        return false;
    }
    auto reply = (redisReply*)redisCommand(conn, "HSET %s %s %s", key.c_str(), field.c_str(), value.c_str());
    if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER) {
        printf("Error: %s\n", conn->errstr);
        freeReplyObject(reply);
        m_redisConnPool->releaseConnection(conn);
        return false;
	}
	std::cout << "HSET " << key << " " << field << " = " << value << ", field was " << (reply->integer == 1 ? "newly added" : "updated") << std::endl;
	freeReplyObject(reply);
    m_redisConnPool->releaseConnection(conn);
    return true;
}

bool RedisMgr::hset(const char* key, const char* field, const char* value, size_t hvaluelen)
{
	const char* argv[4];
	size_t argvlen[4];
	argv[0] = "HSET";
	argvlen[0] = 4;
	argv[1] = key;
	argvlen[1] = strlen(key);
	argv[2] = field;
	argvlen[2] = strlen(field);
	argv[3] = value;
	argvlen[3] = hvaluelen;
    auto conn = m_redisConnPool->getConnection();
    if (conn == nullptr) {
        printf("Error: no available Redis connection\n");
        return false;
    }
    auto reply = (redisReply*)redisCommandArgv(conn, 4, argv, argvlen);
    if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER) {
        printf("Error: %s\n", conn->errstr);
        freeReplyObject(reply);
        m_redisConnPool->releaseConnection(conn);
        return false;
	}
	std::cout << "HSET " << key << " " << field << " = " << value << ", field was " << (reply->integer == 1 ? "newly added" : "updated") << std::endl;
	freeReplyObject(reply);
    m_redisConnPool->releaseConnection(conn);
    return true;
}

std::string RedisMgr::hget(const std::string& key, const std::string& field)
{
	const char* argv[3];
	size_t argvlen[3];
	argv[0] = "HGET";
	argvlen[0] = 4;
	argv[1] = key.c_str();
	argvlen[1] = key.length();
	argv[2] = field.c_str();
	argvlen[2] = field.length();
    auto conn = m_redisConnPool->getConnection();
    if (conn == nullptr) {
        printf("Error: no available Redis connection\n");
        return std::string();
    }
    auto reply = (redisReply*)redisCommandArgv(conn, 3, argv, argvlen);
    if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
        printf("Error: %s\n", conn->errstr);
        freeReplyObject(reply);
        m_redisConnPool->releaseConnection(conn);
        return std::string();
	}
    if (reply->type != REDIS_REPLY_STRING) {
        std::cout << "Error: HGET command failed" << std::endl;
        freeReplyObject(reply);
        m_redisConnPool->releaseConnection(conn);
        return std::string();
	}
	std::string value = reply->str;
	std::cout << "HGET " << key << " " << field << " = " << value << std::endl;
	freeReplyObject(reply);
	m_redisConnPool->releaseConnection(conn);
	return value;

}

bool RedisMgr::del(const std::string& key)
{
    auto conn = m_redisConnPool->getConnection();
    if (conn == nullptr) {
        printf("Error: no available Redis connection\n");
        return false;
    }
    auto reply = (redisReply*)redisCommand(conn, "DEL %s", key.c_str());
    if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER) {
        printf("Error: %s\n", conn->errstr);
        freeReplyObject(reply);
        m_redisConnPool->releaseConnection(conn);
        return false;
	}
	std::cout << "DEL " << key << ", " << reply->integer << " keys were removed" << std::endl;
	freeReplyObject(reply);
	m_redisConnPool->releaseConnection(conn);
    return true;
}

bool RedisMgr::existsKey(const std::string& key)
{
    auto conn = m_redisConnPool->getConnection();
    if (conn == nullptr) {
        printf("Error: no available Redis connection\n");
        return false;
    }
    auto reply = (redisReply*)redisCommand(conn, "EXISTS %s", key.c_str());
    if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER || reply->integer == 0) {
        printf("Error: %s\n", conn->errstr);
        freeReplyObject(reply);
        m_redisConnPool->releaseConnection(conn);
        return false;
	}
	bool exists = (reply->integer == 1);
	std::cout << "EXISTS " << key << " = " << (exists ? "true" : "false") << std::endl;
	freeReplyObject(reply);
	m_redisConnPool->releaseConnection(conn);
    return exists;
}

void RedisMgr::close() {
	m_redisConnPool->close();
}

RedisMgr::RedisMgr()
{
	auto& config = ConfigMgr::getInstance();
	auto host = config["Redis"]["host"];
	auto port = config["Redis"]["port"];
	auto pwd = config["Redis"]["passwd"];
	m_redisConnPool.reset(new RedisConnPool(5, host.c_str(), std::stoi(port), pwd.c_str()));
}

RedisConnPool::RedisConnPool(size_t poolsize, const char* host, int port, const char* pwd)
	: m_stop(false), m_poolsize(poolsize), m_host(host), m_port(port)
{
    for (size_t i = 0; i < m_poolsize; ++i) {
        redisContext* conn = redisConnect(m_host, m_port);
        if (conn == nullptr || conn->err) {
            if (conn) {
                printf("Error: %s\n", conn->errstr);
                redisFree(conn);
            }
            else {
                printf("Can't allocate redis context\n");
            }
            continue;
        }
		auto reply = (redisReply*)redisCommand(conn, "AUTH %s", pwd);
        if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
            printf("Error: %s\n", conn->errstr);
            freeReplyObject(reply);
            redisFree(conn);
            continue;
        }
        freeReplyObject(reply);
		std::cout << "认证成功" << std::endl;
		m_connections.push(conn);
	}
}

RedisConnPool::~RedisConnPool() {
    std::lock_guard<std::mutex> lock(m_mutex);
    while (!m_connections.empty())
    {
        auto conn = m_connections.front();
        m_connections.pop();
        redisFree(conn);
	}
}

void RedisConnPool::close() {
	m_stop = true;
	m_cond.notify_all();
}

redisContext* RedisConnPool::getConnection() {
	std::unique_lock<std::mutex> lock(m_mutex);
    m_cond.wait(lock, [this]() {
        if (m_stop) return true;
        return !m_connections.empty();
		});
    if (m_stop) return nullptr;
	auto conn = m_connections.front();
	m_connections.pop();
	return conn;
}

void RedisConnPool::releaseConnection(redisContext* conn) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_stop) return;
	m_connections.push(conn);
	m_cond.notify_one();
}
