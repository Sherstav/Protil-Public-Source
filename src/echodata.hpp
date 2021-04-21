#pragma once
#include "json/picojson.h"
#include <string>

class EchoData
{
public:
	static std::string Request(const std::string& URL);
	static picojson::value Parse(const std::string& str);
	static bool IsCompleted;
};