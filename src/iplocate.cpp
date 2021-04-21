#include "iplocate.hpp"

#include "echodata.hpp"
#include "json/picojson.h"

/*

Usage:

Location a = IpLocate::Locate("204.74.226.94");

std::cout << a.city.c_str() << std::endl;
std::cout << a.region.c_str() << std::endl;
std::cout << a.country.c_str() << std::endl;
std::cout << a.isp.c_str() << std::endl;

Output:

Dallas
Texas
United States
Servers.com, Inc.

*/

Location IpLocate::Locate(std::string ip)
{
	Location location;

	std::string response = EchoData::Request("http://ip-api.com/json/" + ip);

	picojson::value parsedResponse;

	picojson::parse(parsedResponse, response);

	location.city = parsedResponse.get("city").to_str();
	location.country = parsedResponse.get("country").to_str();
	location.region = parsedResponse.get("regionName").to_str();
	location.isp = parsedResponse.get("isp").to_str();

	return location;
}
