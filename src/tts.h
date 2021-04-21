#pragma once

#include <string>

class tts
{
public:
	static int Speak(const std::string& words); // returns false when failed
	static std::wstring s2ws(const std::string& s);
};