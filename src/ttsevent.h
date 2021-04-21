#pragma once

#include <string>
#include <mutex>
#include <condition_variable>

class beep
{
public:
	static void AddBeepEvent(const int& freq, const int& dur, const int& repeats);
	static void beep_thread_operation();
	static std::condition_variable cv;
private:
	static int stack[3];
	static std::mutex beepmutex;
};

class ttsevent
{
public:
	static void AddSpeakEvent(const std::string& data);
	static void tts_thread_operation();
	static std::condition_variable cv;
private:
	static std::string stack;
	static std::mutex ttsmutex;
};