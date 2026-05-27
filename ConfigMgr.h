#pragma once
#include <map>
#include <string>

struct SectionInfo
{
	std::map<std::string, std::string> m_section_datas;

	SectionInfo() = default;
	~SectionInfo() {
		m_section_datas.clear();
	}

	SectionInfo(const SectionInfo& other) {
		m_section_datas = other.m_section_datas;
	}

	SectionInfo& operator=(const SectionInfo& other) {
		if (this != &other) {
			m_section_datas = other.m_section_datas;
		}
		return *this;
	}

	std::string operator[](const std::string& key) {
		if (m_section_datas.find(key) == m_section_datas.end()) return "";
		return m_section_datas[key];
	}
};

class ConfigMgr
{
public:
	~ConfigMgr() {
		m_config_map.clear();
	}
	
	SectionInfo operator[](const std::string& section) {
		if (m_config_map.find(section) == m_config_map.end()) return SectionInfo();
		return m_config_map[section];
	}

	//ConfigMgr(const ConfigMgr& other) {
	//	m_config_map = other.m_config_map;
	//}

	//ConfigMgr& operator=(const ConfigMgr& other) {
	//	if (this != &other) {
	//		m_config_map = other.m_config_map;
	//	}
	//	return *this;
	//}

	ConfigMgr(const ConfigMgr&) = delete;
	ConfigMgr& operator=(const ConfigMgr&) = delete;

	static ConfigMgr& getInstance() {
		static ConfigMgr instance;
		return instance;
	}

private:
	ConfigMgr();
	std::map<std::string, SectionInfo> m_config_map;
};

