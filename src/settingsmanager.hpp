#pragma once
#include "json/picojson.h"
#include <string>


class SettingsManager
{
public:
	static picojson::value GetSettings();
	static void CreateNewSettingsString(const std::string& name, const std::string& data);
	static void CreateNewSettingsBool(const std::string& name, const bool& data);
	static void CreateNewSettingsFloat(const std::string& name, const float& data);
	static void WriteSettingsToFile();
	//static void CreateNewSettingsFloat(const std::string& name, const std::string& data);
private:
	static FILE* file;
};

