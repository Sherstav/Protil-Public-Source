#include <iostream>
#include <string>
#include "loop.hpp"
#include "window.hpp"
#include "echodata.hpp"
#include "json/picojson.h"
#include "tts.h"
#include "settingsmanager.hpp"
#include <thread>
#include <chrono>
#include <mutex>
#include "STDAfx.h"
#include "discordrp.h"

bool Loop::running = true;

#ifdef NDEBUG
#include <Windows.h>

// char Window::s_headsetIP[15] = SettingsManager::GetSettings().get("headsetIP").to_str().c_str();

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	PSTR lpCmdLine, INT nCmdShow)
{

	Window window = Window();

	Loop::running = true;

	while (Loop::running)
	{
		window.Render();
		std::this_thread::yield();
	}

	return 0;
}
#endif

picojson::value parsed;

#ifdef _DEBUG

int main(int argc, char* argv[])
{
	Window window = Window();

	for (int i = 0; i < argc; i++)
	{
		std::cout << argv[i] << std::endl;
		if (std::string(argv[i]) == "-changelog")
		{
			std::cout << "Showing changelog" << std::endl;
			window.showCheckForUpdates = true;
		}
	}

	Loop::running = true;

	while (Loop::running)
	{

		window.Render();
		std::this_thread::yield();
	}

    return 0;
}

#endif 