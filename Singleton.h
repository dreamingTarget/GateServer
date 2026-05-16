#pragma once
#include <memory>
#include <mutex>
#include <iostream>

template<class T>
class Singleton
{
public:
	static std::shared_ptr<T> getInstance()
	{
		static std::once_flag flag;
		std::call_once(flag, []() {
			m_instance = std::shared_ptr<T>(new T());
			});
		return m_instance;
	}

	void printAddress()
	{
		std::cout << "instance address is " << m_instance.get() << std::endl;
	}

	~Singleton() = default;

protected:
	Singleton() = default;
	Singleton(const Singleton&) = delete;
	Singleton& operator=(const Singleton&) = delete;

protected:
	static std::shared_ptr<T> m_instance;
};

template<class T>
std::shared_ptr<T> Singleton<T>::m_instance = nullptr;
