#pragma once

#include <string>

struct Location
{
	std::string city;
	std::string region;
	std::string country;
	std::string isp;
};

class IpLocate
{
public:
	static Location Locate(std::string ip);
};