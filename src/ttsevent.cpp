#include "ttsevent.h"
#include "loop.hpp"
#include "tts.h"
#include <chrono>
#include <windows.h>


std::string ttsevent::stack = "";
std::mutex ttsevent::ttsmutex = std::mutex();
std::condition_variable ttsevent::cv = std::condition_variable();

void ttsevent::AddSpeakEvent(const std::string& data)
{
	std::unique_lock<std::mutex> lck(ttsmutex);
	stack = data;
	cv.notify_one();
}

void ttsevent::tts_thread_operation()
{
	
	while (Loop::running)
	{
		std::unique_lock<std::mutex> lck(ttsmutex);
		cv.wait(lck);
		if (!stack.empty())
		{
			tts::Speak(stack);
			stack = "";
		}
	}
}

int beep::stack[3] = {0,0,0};
std::mutex beep::beepmutex = std::mutex();
std::condition_variable beep::cv = std::condition_variable();
void beep::AddBeepEvent(const int& freq, const int& dur, const int& repeats)
{
	std::unique_lock<std::mutex> lck(beepmutex);
	stack[0] = freq;
	stack[1] = dur;
	stack[2] = repeats;
}

void beep::beep_thread_operation()
{
	while (Loop::running)
	{
		std::unique_lock<std::mutex> lck(beepmutex);
		cv.wait(lck);
		if (stack[0] != 0)
		{
			for (int i = 0; i < stack[2]; i++)
			{
				Beep(stack[0], stack[1]);
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}
			stack[0] = 0;
			stack[1] = 0;
			stack[2] = 0;
		}
	}
}
