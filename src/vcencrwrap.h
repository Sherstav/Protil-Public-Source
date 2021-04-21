#pragma once
#include <string>
#include "voidcrypt/voidcrypt.h"

struct encrwrap
{
	static std::string perform(const std::string& data);
	static const char* bkey;
};