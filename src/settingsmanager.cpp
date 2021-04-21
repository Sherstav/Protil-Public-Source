#include "settingsmanager.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdio.h>

FILE* SettingsManager::file = nullptr;

picojson::value SettingsManager::GetSettings()
{
	//file = fopen("settings.json", "r");
	errno_t openErr = fopen_s(&file, "settings.json", "r");

	size_t string_size = 0;

	fseek(file, 0, SEEK_END);
	string_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* tmp_string = new char[string_size];
	fread(tmp_string, string_size, 1, file);

	std::string str = std::string(tmp_string, string_size);

	delete tmp_string;
	std::string err;
	picojson::value returnVal;
	picojson::parse(returnVal, str.begin(), str.end(), &err);

	fclose(file);

	return returnVal;
}

picojson::object obj;

void SettingsManager::CreateNewSettingsString(const std::string& name, const std::string& data)
{
	obj[name] = picojson::value(data);
	std::cout << data << std::endl; 
}

void SettingsManager::CreateNewSettingsBool(const std::string& name, const bool& data)
{
	obj[name] = picojson::value(data);
}

void SettingsManager::CreateNewSettingsFloat(const std::string& name, const float& data)
{
	obj[name] = picojson::value(data);
}
void SettingsManager::WriteSettingsToFile()
{
	std::remove("./settings.json");
	std::ofstream fileHandle;
	fileHandle.open("settings.json");

	fileHandle << picojson::value(obj);

	fileHandle.close();
}
//
//void SettingsManager::CreateNewSettingsFloat(const std::string& name, const std::string& data)
//{
//	obj[name] = picojson::value(data);
//}
