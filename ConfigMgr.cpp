#include "ConfigMgr.h"
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <iostream>

ConfigMgr::ConfigMgr()
{
	boost::filesystem::path cur_path = boost::filesystem::current_path();
	boost::filesystem::path config_path = cur_path / "config.ini";
	std::cout << "config path: " << config_path.string() << std::endl;

	boost::property_tree::ptree pt;
	boost::property_tree::ini_parser::read_ini(config_path.string(), pt);

	for (const auto& section : pt) {
		std::string section_name = section.first;
		SectionInfo section_info;
		for (const auto& key_value : section.second) {
			std::string key = key_value.first;
			std::string value = key_value.second.get_value<std::string>();
			section_info.m_section_datas[key] = value;
		}
		m_config_map[section_name] = section_info;
	}
}
