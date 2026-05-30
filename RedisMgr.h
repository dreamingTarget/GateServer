#pragma once
#include "Singleton.h"
#include "hiredis.h"
#include <queue>

class RedisConnPool
{
	public:
	RedisConnPool(size_t poolsize, const char* host, int port, const char* pwd);
	~RedisConnPool();
	void close();
	redisContext* getConnection();
	void releaseConnection(redisContext* conn);

private:
	std::atomic<bool> m_stop;
	size_t m_poolsize;
	int m_port;
	const char* m_host;
	std::queue<redisContext*> m_connections;
	std::mutex m_mutex;
	std::condition_variable m_cond;
};

class RedisMgr : public Singleton<RedisMgr>
{
	friend class Singleton<RedisMgr>;
public:
	~RedisMgr();
	//bool connect(const std::string& ip, int port);
	bool get(const std::string& key, std::string& value);
	bool set(const std::string& key, const std::string& value);
	bool auth(const std::string& password);
	bool lpush(const std::string& key, const std::string& value);
	bool lpop(const std::string& key, std::string& value);
	bool rpush(const std::string& key, const std::string& value);
	bool rpop(const std::string& key, std::string& value);
	bool hset(const std::string& key, const std::string& field, const std::string& value);
	bool hset(const char* key, const char* field, const char* value, size_t hvaluelen);
	std::string hget(const std::string& key, const std::string& field);
	bool del(const std::string& key);
	bool existsKey(const std::string& key);
	void close();
	
private:
	RedisMgr();

private:
	//redisContext* m_redisContext;
	//redisReply* m_redisReply;
	std::unique_ptr<RedisConnPool> m_redisConnPool;
};

