#pragma once

#include <string>
#include <mutex>
#include <thread>
#include "discordrp.h"
#include "settingsmanager.hpp"
#include "iplocate.hpp"
#include "voidcrypt/voidcrypt.h"
#include "vcencrwrap.h"
typedef struct SDL_Window SDL_Window;
struct SDL_Renderer;
typedef void* SDL_GLContext;
struct ImGuiIO;

void data_thread_func();

class Window
{
public:
	Window();
	~Window();
	void Render();
private:
	const int version = 2;

	void UpdateTheme();
	void DarkTheme();
	void CherryTheme();
	void GreyTheme();
	void SourceTheme();
	void GruvboxTheme();
	picojson::value GetTeamPlayer(int player, int team);
	picojson::value FindPlayer(const std::string& name);
	int FindPlayerTeam(const std::string& name);
	std::string CheckForUpdates();
	void UpdateVersion();
	SDL_Window* m_window;
	SDL_Renderer* m_renderer;
	SDL_GLContext m_gl_context;
	ImGuiIO* m_io;
	bool windowOpen;
	std::string m_status;
	std::thread data_thread;
	std::thread tts_thread;
	std::thread beep_thread;
	std::string m_last_map;
	picojson::value m_clientplayerdata = picojson::value();
	Location m_last_location;
	std::string m_last_sessionid;
	bool enableBeep;
	bool firstStart = true;
	std::string status;
	int multiplier = 340;
	bool m_show_update_menu;
	std::string psdata = "";
	int fc = 1;
	float posX = 0, posY = 0, posZ = 0;
	std::string psreaddata = "";
	picojson::value psreadvalue = picojson::value();
	bool isrecordopen = false;
	std::string changelog = "";
	bool newupdate = false;
	bool customJoinMessage;
	int latestVersion;
	bool isgreenscreen = false;
	bool wasingame = false;
	picojson::value lastmatchdata = picojson::value();
public:
	static const char* key;
	static bool recording;
	static bool alreadybeeped;
	bool showPlaySpaceVisualizer;
	bool showPSRecordingViewer;
	bool showFAQ;
	bool showCheckForUpdates;
	static DiscordRP discordRP;
	static bool isInGame;
	bool showPingGraph;
	bool isTTS;
	bool useDiscordRP;
	static std::string s_serverLocation;
	static std::mutex s_mutex;
	static char s_headsetIP[15];
	static char s_gamertag[100];
	static char customMessage[100];
	static picojson::value s_echodata;
	std::string theme;
	static std::string s_ping;
};