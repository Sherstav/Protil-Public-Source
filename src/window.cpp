#include "window.hpp"
#include "SDL/SDL.h"
#include "glad/glad.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"
#include "loop.hpp"
#include "settingsmanager.hpp"
#include "echodata.hpp"
#include "tts.h"
#include <iostream>
#include <math.h>
#include <vector>
#include "ttsevent.h"
#include <windows.h>
#include <shellapi.h>
#include <filesystem>
#include <shlobj.h>
#include <windows.h>
#include <Lmcons.h>
#include "imgloader.h"
#include "stb_image/stb_image.h"
#include "windowsdialog.h"
#include <ctime>
#include <fstream>

bool Window::alreadybeeped = false;
char Window::customMessage[100];
char Window::s_headsetIP[15];
char Window::s_gamertag[100];
std::mutex Window::s_mutex = std::mutex();
std::string Window::s_serverLocation = "";
std::string Window::s_ping = "";
picojson::value Window::s_echodata = picojson::value();
bool Window::isInGame = false;
DiscordRP Window::discordRP = DiscordRP();

int border_image_width = 0;
int border_image_height = 0;
GLuint border_image_texture = 0;
bool ret;

int player_image_width = 0;
int player_image_height = 0;
GLuint player_image_texture = 0;
bool retp;

Window::Window()
{

	beep_thread = std::thread(beep::beep_thread_operation);
	data_thread = std::thread(data_thread_func);
	tts_thread = std::thread(ttsevent::tts_thread_operation);

	/*for (int i = 0; i < sizeof(s_pingGraph) / sizeof(s_pingGraph[0]); i++)
	{
		s_pingGraph[i] = 0;
	}*/

	/*recieves the settings*/
	customJoinMessage = SettingsManager::GetSettings().get("customJoinMessage").evaluate_as_boolean();
	firstStart = SettingsManager::GetSettings().get("firstStart").evaluate_as_boolean();
	enableBeep = SettingsManager::GetSettings().get("enableBeep").evaluate_as_boolean();
	isTTS = SettingsManager::GetSettings().get("isTTS").evaluate_as_boolean();
	showPingGraph = SettingsManager::GetSettings().get("showPingGraph").evaluate_as_boolean();
	useDiscordRP = SettingsManager::GetSettings().get("useDiscordRP").evaluate_as_boolean();

	std::string custommsgtemp = SettingsManager::GetSettings().get("customMessage").to_str();
	for (int i = 0; i < custommsgtemp.length(); i++)
		customMessage[i] = custommsgtemp[i];

	std::string teststrtochar = SettingsManager::GetSettings().get("headsetIP").to_str();
	for (int i = 0; i < teststrtochar.length(); i++)
		s_headsetIP[i] = teststrtochar[i];

	theme = SettingsManager::GetSettings().get("theme").to_str();

	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	m_window = SDL_CreateWindow("Protils", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280/1.15, 720/1.25, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);


	// Window settings

	m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);

	if (m_renderer == nullptr) {
		std::cerr << "SDL2 Renderer couldn't be created. Error: " << SDL_GetError() << std::endl;
		exit(1);
	}

	m_gl_context = SDL_GL_CreateContext(m_window);

	// Load GL extensions using glad
	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
		std::cerr << "Failed to initialize the OpenGL context." << std::endl;
		exit(1);
	}

	SDL_DisplayMode dm;
	SDL_GetCurrentDisplayMode(0, &dm);

	// Imgui init

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& m_io = ImGui::GetIO(); (void)m_io;

	m_io.DisplaySize = ImVec2(dm.w, dm.h);
	m_io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 16.0f, NULL, NULL);
	m_io.ConfigWindowsMoveFromTitleBarOnly = true;
	m_io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	m_io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	m_io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	// Setup Dear Imgui style

	UpdateTheme();

	// Setup platform/Renderer bindings
	ImGui_ImplSDL2_InitForOpenGL(m_window, m_gl_context);
	ImGui_ImplOpenGL3_Init();

	SDL_GL_SetSwapInterval(1);

	windowOpen = true;

	m_last_sessionid = "";
	int req_format = STBI_rgb_alpha;
	int width, height, orig_format;
	unsigned char* data = stbi_load("img/sherstav.png", &width, &height, &orig_format, req_format);
	Uint32 rmask, gmask, bmask, amask;
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = (req_format == STBI_rgb) ? 0 : 0xff000000;

	int depth, pitch;
	if (req_format == STBI_rgb) {
		depth = 24;
		pitch = 3 * width; // 3 bytes per pixel * pixels per row
	}
	else { // STBI_rgb_alpha (RGBA)
		depth = 32;
		pitch = 4 * width;
	}
	SDL_Surface* surf = SDL_CreateRGBSurfaceFrom((void*)data, width, height, depth, pitch,
		rmask, gmask, bmask, amask);
	SDL_SetWindowIcon(m_window, surf);
	ret = imgloader::LoadTextureFromFile("img/border.png", &border_image_texture, &border_image_width, &border_image_height);
	retp = imgloader::LoadTextureFromFile("img/player.png", &player_image_texture, &player_image_width, &player_image_height);

	UpdateVersion();

	if (latestVersion > version)
	{
		showCheckForUpdates = true;
	}
}

const std::string currentDateTime() {
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

	return buf;
}

Window::~Window()
{
	SDL_GL_DeleteContext(m_gl_context);
	SDL_DestroyRenderer(m_renderer);
	SDL_DestroyWindow(m_window);

	SDL_Quit();
}

bool showSettings = false;
bool showJoinAtlas = false;
bool showPlayerStats = false;

std::string data;
bool Window::recording = false;

void data_thread_func()
{
	while (Loop::running)
	{
		data = EchoData::Request(Window::s_headsetIP + std::string(":6721/session"));

		if (!data.empty() && data != "<html><head><title>404 Not Found</title></head><body><h1>Not Found</h1><p>The requested URL /session was not found on this server.</p></body></html>")
		{
			picojson::value v;
			picojson::parse(v, data);
			if (v.to_str() != "null")
			{
				Window::isInGame = true;
				Window::s_mutex.lock();
				Window::s_echodata = v;
				Window::s_mutex.unlock();
			}
		}
		else
			Window::isInGame = false;
		std::this_thread::sleep_for(std::chrono::milliseconds(25));
	}
}

std::string Window::CheckForUpdates()
{
	return EchoData::Request("https://chaugh.com/status/EchoUtil/latestversion");
}

void Window::UpdateVersion()
{
	latestVersion = std::stoi(CheckForUpdates());
	changelog = EchoData::Request("https://chaugh.com/status/EchoUtil/changelog");
}

picojson::value Window::GetTeamPlayer(int player, int team)
{
	return s_echodata.get("teams").get(team).get("players").get(player);
}

picojson::value Window::FindPlayer(const std::string& name)
{
	// cycle through team0
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < s_echodata.get("teams").get(i).get("players").get<picojson::array>().size(); j++)
		{
			picojson::value currentPlayer = s_echodata.get("teams").get(i).get("players").get(j);

			if (currentPlayer.get("name").to_str() == name)
			{
				return currentPlayer;
			}
		}
	}
	return picojson::value();
}

int Window::FindPlayerTeam(const std::string& name)
{
	// cycle through team0
	for (int i = 0; i < 3; i++)
	{
		if (s_echodata.get("teams").get(i).get("players").to_str() != "null")
		{
			for (int j = 0; j < s_echodata.get("teams").get(i).get("players").get<picojson::array>().size(); j++)
			{
				picojson::value currentPlayer = s_echodata.get("teams").get(i).get("players").get(j);

				if (currentPlayer.get("name").to_str() == name)
				{
					return i;
				}
			}
		}
	}
	return -1;
}
void TextCenter(std::string text) {
	float font_size = ImGui::GetFontSize() * text.size() / 2;
	ImGui::SameLine(
		ImGui::GetWindowSize().x / 2 -
		font_size + (font_size / 2)
	);

	ImGui::Text(text.c_str());
}
void Window::Render()
{
	float starttime = SDL_GetTicks() / 1000.0f;
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		if (event.type == SDL_QUIT)
		{
			Loop::running = false;
			std::string s(SDL_GetWindowTitle(m_window));
			SDL_SetWindowTitle(m_window, (s + " (Closing)").c_str());
			ttsevent::cv.notify_all();
			beep::cv.notify_all();
			tts_thread.join();
			data_thread.join();
			beep_thread.join();
		}
		ImGui_ImplSDL2_ProcessEvent(&event);
	}

	glClearColor(ImGui::GetStyle().Colors[ImGuiCol_WindowBg].x, ImGui::GetStyle().Colors[ImGuiCol_WindowBg].y, ImGui::GetStyle().Colors[ImGuiCol_WindowBg].z, ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w);
	glClear(GL_COLOR_BUFFER_BIT);

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(m_window);
	ImGui::NewFrame();
	ImGui::DockSpaceOverViewport((ImGuiViewport*)0, ImGuiDockNodeFlags_AutoHideTabBar);

	if (firstStart)
	{
		ImGui::Text(changelog.c_str());
		if (ImGui::Button("OK"))
		{
			firstStart = false;
			SettingsManager::CreateNewSettingsBool("firstStart", firstStart);
			SettingsManager::WriteSettingsToFile();
		}
	}

	int w;
	int h;
	SDL_GetWindowSize(m_window, &w, &h);

	ImGui::BeginMainMenuBar();
	ImGui::MenuItem("Settings", (const char*)0, &showSettings);
	if (ImGui::BeginMenu("Tools"))
	{
		ImGui::MenuItem("Join Atlas Link", (const char*)0, &showJoinAtlas);
		ImGui::MenuItem("Playspace Visualizer", (const char*)0, &showPlaySpaceVisualizer);
		ImGui::MenuItem("Playspace Recording Viewer", (const char*)0, &showPSRecordingViewer);
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Help"))
	{
		ImGui::MenuItem("FAQ", (const char*)0, &showFAQ);
		bool justEnabledCheckForUpdates = showCheckForUpdates;
		ImGui::MenuItem("Check for Updates", (const char*)0, &showCheckForUpdates);

		if (!justEnabledCheckForUpdates && showCheckForUpdates)
		{
			std::cout << "Checking for updates..." << std::endl;
			UpdateVersion();
		}
		ImGui::EndMenu();
	}
	//ImGui::MenuItem("Help", (const char*)0, &showHelpMenu);
	ImGui::EndMainMenuBar();

	//ImGui::Begin("Main Menu", nullptr, ImGuiWindowFlags_NoBackground);

	if (isInGame) //checks if the game is open
	{
		s_mutex.lock();

		if (!Window::discordRP.coreExists && useDiscordRP)
		{
			Window::discordRP.CreateCore();
		}
		if (Window::discordRP.coreExists && !useDiscordRP)
		{
			Window::discordRP.DeleteCore();
		}
		if (Window::discordRP.coreExists)
			Window::discordRP.Update();
		if (s_echodata.get("map_name").to_str() != m_last_map)
		{
			m_last_map = s_echodata.get("map_name").to_str();

			m_last_sessionid = s_echodata.get("sessionid").to_str();

			m_last_location = IpLocate::Locate(s_echodata.get("sessionip").to_str());

			std::string message = "";

			if(!customJoinMessage)
				ttsevent::AddSpeakEvent("Server location is " + m_last_location.city + ", " + m_last_location.region + ", in " + m_last_location.country + ", under: " + m_last_location.isp);
			else
			{
				for (int i = 0; i < sizeof(customMessage); i++)
				{
					if (customMessage[i] == '{')
					{
						bool done = false;
						int ct = 0;
						std::string arg = "";
						while (!done)
						{
							ct++;
							if (customMessage[i + ct] != '}')
							{
								arg += customMessage[i + ct];
							}
							else if (customMessage[i + ct] == '}')
							{
								done = true;
								i += ct;
							}
						}
						if (arg == "city")
							message += m_last_location.city;
						else if (arg == "region")
							message += m_last_location.region;
						else if (arg == "country")
							message += m_last_location.country;
						else if (arg == "isp")
							message += m_last_location.isp;
					} 
					else
						message += customMessage[i];
				}
				ttsevent::AddSpeakEvent(message);
			}
		}
		
		if (Window::discordRP.coreExists)
		{
			if (s_echodata.get("map_name").to_str() == "mpl_lobby_b2")
			{
				Window::discordRP.m_activity.SetDetails("In Lobby");
			}

			if (s_echodata.get("map_name").to_str() == "mpl_arena_a")
			{
				if (s_echodata.get("private_match").evaluate_as_boolean())
				{
					if (FindPlayerTeam(s_echodata.get("client_name").to_str()) == 2)
					{
						Window::discordRP.m_activity.SetDetails("Spectating Private Match");
					}
					else
					{
						Window::discordRP.m_activity.SetDetails("Playing Private Match");
					}
					Window::discordRP.m_activity.GetAssets().SetSmallImage("lockedlock");
					Window::discordRP.m_activity.GetAssets().SetSmallText("Private");
				}
				else
				{
					if (FindPlayerTeam(s_echodata.get("client_name").to_str()) == 2)
					{
						Window::discordRP.m_activity.SetDetails("Spectating Public Match");
					}
					else
					{
						Window::discordRP.m_activity.SetDetails("Playing Public Match");
					}

					Window::discordRP.m_activity.GetAssets().SetSmallImage("unlockedlock");
					Window::discordRP.m_activity.GetAssets().SetSmallText("Public");
				}

				Window::discordRP.m_activity.SetState(s_echodata.get("game_status").to_str().c_str());
			}
		}

		if (s_echodata.get("map_name").to_str() == "mpl_arena_a")
		{
			wasingame = true;
			lastmatchdata = s_echodata;

			int team0size = 0;
			int team1size = 0;
			if(s_echodata.get("teams").get(0).get("players").to_str() != "null")
				team0size = s_echodata.get("teams").get(0).get("players").get<picojson::array>().size();
			if(s_echodata.get("teams").get(1).get("players").to_str() != "null")
				team1size = s_echodata.get("teams").get(1).get("players").get<picojson::array>().size();

			ImGui::Begin("Match Player Data");

			bool isTableOpen = ImGui::BeginTable("players", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);

			ImGui::TableNextColumn();
			ImGui::Separator();

			ImGui::TextColored(ImVec4(230.0f / 255.0f, 126.0f / 255.0f, 34.0f / 255.0f, 1),s_echodata.get("teams").get(1).get("team").to_str().c_str());

			ImGui::TableNextColumn();
			ImGui::Separator();

			ImGui::TextColored(ImVec4(52.0f / 255.0f, 152.0f / 255.0f, 219.0f / 255.0f, 1), s_echodata.get("teams").get(0).get("team").to_str().c_str());

			for (int i = 0; i < 5; i++)
			{
				// team 0 person
				static float s_pingGraph[10][5000];

				ImGui::TableNextColumn();
				ImGui::Separator();

				if (team1size > i)
				{
					picojson::value player = s_echodata.get("teams").get(1).get("players").get(i);

					ImGui::Text(player.get("name").to_str().c_str());
					ImGui::Text("Level: ");
					ImGui::SameLine();
					ImGui::Text(player.get("level").to_str().c_str());

					ImGui::Text("Points: ");
					ImGui::SameLine();
					ImGui::Text(player.get("stats").get("points").to_str().c_str());

					ImGui::Text("Steals: ");
					ImGui::SameLine();
					ImGui::Text(player.get("stats").get("steals").to_str().c_str());

					ImGui::Text("Stuns: ");
					ImGui::SameLine();
					ImGui::Text(player.get("stats").get("stuns").to_str().c_str());

					ImGui::Text("Saves: ");
					ImGui::SameLine();
					ImGui::Text(player.get("stats").get("saves").to_str().c_str());

					if (showPingGraph)
					{
						ImGui::Text("Ping: ");

						s_pingGraph[i + 5][4999] = std::stoi(player.get("ping").to_str(), nullptr, 10);
						float pinggraphdup[5000];
						for (int j = 0; j < 5000; j++)
							pinggraphdup[j] = s_pingGraph[i + 5][j];
						for (int k = 0; k < 5000; k++)
						{
							if (k != 0)
								s_pingGraph[i + 5][k - 1] = pinggraphdup[k];
						}

						ImGui::PlotLines("", s_pingGraph[i + 5], sizeof(s_pingGraph[i + 5]) / sizeof(s_pingGraph[i + 5][0]), 0, player.get("ping").to_str().c_str(), FLT_MIN, FLT_MAX, ImVec2(200, 50), sizeof(float));
					}
					else
					{
						ImGui::Text("Ping: ");
						ImGui::SameLine();
						ImGui::Text(player.get("ping").to_str().c_str());
					}
				}

				ImGui::TableNextColumn();
				ImGui::Separator();

				if (team0size > i)
				{
					picojson::value player = s_echodata.get("teams").get(0).get("players").get(i);

					ImGui::Text(player.get("name").to_str().c_str());
					ImGui::Text("Level: ");
					ImGui::SameLine();
					ImGui::Text(player.get("level").to_str().c_str());

					ImGui::Text("Points: ");
					ImGui::SameLine();
					ImGui::Text(player.get("stats").get("points").to_str().c_str());

					ImGui::Text("Steals: ");
					ImGui::SameLine();
					ImGui::Text(player.get("stats").get("steals").to_str().c_str());

					ImGui::Text("Stuns: ");
					ImGui::SameLine();
					ImGui::Text(player.get("stats").get("stuns").to_str().c_str());

					ImGui::Text("Saves: ");
					ImGui::SameLine();
					ImGui::Text(player.get("stats").get("saves").to_str().c_str());

					if (showPingGraph)
					{
						ImGui::Text("Ping: ");

						s_pingGraph[i][4999] = std::stoi(player.get("ping").to_str(), nullptr, 10);
						float pinggraphdup[5000];
						for (int j = 0; j < 5000; j++)
							pinggraphdup[j] = s_pingGraph[i][j];
						for (int k = 0; k < 5000; k++)
						{
							if (k != 0)
								s_pingGraph[i][k - 1] = pinggraphdup[k];
						}

						ImGui::PlotLines("", s_pingGraph[i], sizeof(s_pingGraph[i]) / sizeof(s_pingGraph[i][0]), 0, player.get("ping").to_str().c_str(), FLT_MIN, FLT_MAX, ImVec2(200, 50), sizeof(float));
					}
					else
					{
						ImGui::Text("Ping: ");
						ImGui::SameLine();
						ImGui::Text(player.get("ping").to_str().c_str());
					}
				}
			}

			if (isTableOpen)
				ImGui::EndTable();

			ImGui::End();

			ImGui::Begin("Match Info");

			ImGui::Text(("Server IP: " + s_echodata.get("sessionip").to_str()).c_str());
			ImGui::Text(("Country: " + m_last_location.country).c_str());
			ImGui::Text(("Region: " + m_last_location.region).c_str());
			ImGui::Text(("City: " + m_last_location.city).c_str());
			ImGui::Text(("Holder: " + m_last_location.isp).c_str());

			ImGui::Separator();
			ImGui::Text("Atlas Join Link");

			std::string tempstr = "atlas://" + m_last_sessionid;

			char joinlink[44];

			for (int i = 0; i < tempstr.length(); i++)
			{
				joinlink[i] = tempstr[i];

			}

			ImGui::InputText("", joinlink, IM_ARRAYSIZE(joinlink), ImGuiInputTextFlags_ReadOnly);
			//ImGui::Selectable(joinlink, false);
			ImGui::SameLine();
			if (ImGui::Button("Copy"))
			{
				ImGui::SetClipboardText(tempstr.c_str());
			}

			ImGui::End();
		}
		else
		{
			//todo: add match recaps vvvvv
			if (/*this just checks if they have been in an arena match*/wasingame)
			{

			}
		}

		if (Window::discordRP.coreExists)
			Window::discordRP.RunCallbacks();
		s_mutex.unlock();
	}
	else
	{
		if (Window::discordRP.coreExists)
		{
			Window::discordRP.DeleteCore();
		}

		m_last_map = "null";

		if (m_last_sessionid != "")
		{
			// user used to be in a game
			
			ImGui::Begin("Rejoin");

			if (ImGui::Button("Rejoin last game"))
			{
				std::wstring a = tts::s2ws(std::string("atlas://") + m_last_sessionid);
				LPCWSTR result = a.c_str();
				ShellExecute(0, 0, result, 0, 0, SW_SHOW);
			}

			ImGui::End();
		}
	}
	bool failed = true;

	bool justStarted = false;
	bool justStopped = false;
	bool lastrecord = recording;
	static ImVec4 bgcol;
	if (showPlaySpaceVisualizer)
	{

		if (isgreenscreen)ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 177.0f / 255.0f, 64.0f / 255.0f, 1));
		ImGui::Begin("Playspace Visualizer", &showPlaySpaceVisualizer);

		s_mutex.lock();
		if (isInGame && FindPlayerTeam(s_echodata.get("client_name").to_str()) != 2 && s_echodata.get("map_name").to_str() != "mpl_lobby_b2")
		{
			//ImGui::Image();

			float X, Y, Z;
			X = stof(s_echodata.get("player").get("vr_position").get<picojson::array>()[0].to_str());
			Y = stof(s_echodata.get("player").get("vr_position").get<picojson::array>()[1].to_str());
			Z = stof(s_echodata.get("player").get("vr_position").get<picojson::array>()[2].to_str());

			if (!recording)
			{
				if (ImGui::Button("Start Recording"))
				{
					fc = 0;
					psdata = "";
					psdata += "[";
					recording = true;
					time_t now = time(0);
					tm* ltm = localtime(&now);
					psdata += /*super secret file layout stuff*/"";
				}
			}
			else
			{
				psdata += /*super secret file layout stuff*/"";
				fc++;
				if (ImGui::Button("Stop Recording") || justStopped)
				{
					recording = false;
					time_t now = time(0); 
					tm* ltm = localtime(&now);
					FILE* file;

					errno_t err;

					err = fopen_s(&file, ("res/" + currentDateTime() + ".psr").c_str(), "wb");
					psdata.pop_back();
					psdata += "]";
					std::string encrdata = encrwrap::perform(psdata);
					fwrite(encrdata.c_str(), 1, encrdata.size(), file);

					fclose(file);

				}
			}

			ImGui::Text(("X: " + std::to_string(X)).c_str());
			ImGui::Text(("Y: " + std::to_string(Y)).c_str());
			ImGui::Text(("Z: " + std::to_string(Z)).c_str());

			IM_ASSERT(ret);
			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - border_image_width) * 0.5f, (ImGui::GetWindowSize().y - border_image_height) * 0.5f));
			ImGui::Image((void*)(intptr_t)border_image_texture, ImVec2(border_image_width, border_image_height));

			ImGui::SetCursorPos(ImVec2(((ImGui::GetWindowSize().x - player_image_width) * 0.5f) - (X * multiplier), ((ImGui::GetWindowSize().y - player_image_height) * 0.5f) - (Z * multiplier)));
			ImGui::Image((void*)(intptr_t)player_image_texture, ImVec2(player_image_width, player_image_height));


			if (enableBeep)
			{
				if (X > .4f || X < -.4f || Z > .4f || Z < -.4f)
				{
					if (!alreadybeeped)
					{
						beep::AddBeepEvent(1500, 100, 2);
						beep::cv.notify_all();
						alreadybeeped = true;
					}
				}
				else
				{
					alreadybeeped = false;
				}
			}
		}
		else
		{
			ImGui::Text("You must be in a game for this tool to be active!");
		}
		s_mutex.unlock();
		if (isgreenscreen)ImGui::PopStyleColor();
		ImGui::End();

	}
	static bool playing;
	static bool showData;
	if (showPSRecordingViewer)
	{
		ImGui::Begin("Playspace Recording Veiwer", &showPSRecordingViewer);
		if (!isrecordopen)
		{
			if (ImGui::Button("Open Playspace File"))
			{
				ImGui::Text(status.c_str());
				std::string path = GetFileLocFromDialog();

				std::streampos size;
				char* memblock;
				std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);
				if (file.is_open())
				{
					size = file.tellg();
					memblock = new char[size];
					file.seekg(0, std::ios::beg);
					file.read(memblock, size);
					file.close();
					std::string result(memblock, size);
					psreaddata = encrwrap::perform(result);
				}
				//run file checks
				const char* datatocheck = psreaddata.c_str();
				std::string yeet = picojson::parse(psreadvalue, datatocheck, datatocheck + strlen(datatocheck));
				if (!yeet.empty())
					status = "Unable to open file do to corrupted file or tampered file";
				else
				{
					playing = false;
					isrecordopen = true;
				}
			}
		}
		else
		{
			int fps = stoi(psreadvalue.get(0).get(/*super secret file layout stuff*/"").to_str());
			int framewait = 1000.0f / fps;
			int timelength = (psreadvalue.get<picojson::array>().size() - 1) / fps;
			if (!playing)
			{
				if (ImGui::Button("Play"))
				{
					playing = true;
					std::cout << psreaddata;
				}
			}
			else
			{
				if (ImGui::Button("-5s"))
				{
					//check if the starting point is within 5 seconds
					if (1 > fc - (5 * fps))
						fc = 1;
					else
						fc = fc - (5 * fps);
				}
				ImGui::SameLine();
				if (ImGui::Button("Pause"))
					playing = false;
				ImGui::SameLine();
				if (ImGui::Button("+5s"))
				{
					//check if the ending point is withing 5 seconds
					if (psreadvalue.get<picojson::array>().size() < fc + (5*fps))
						fc = psreadvalue.get<picojson::array>().size();
					else
						fc += (5 * fps);
				}
				ImGui::SameLine();
				ImGui::Text((std::to_string(fc/fps) + "/" + std::to_string(timelength) + "s").c_str());
			}
			ImGui::SameLine();
			if (ImGui::Button("Show Data")) showData = true;
			if (showData)
			{
				ImGui::Begin("PSR data", &showData);
				ImGui::Text(("Date Recorded: " + psreadvalue.get(0).get(/*super secret file layout stuff*/"").to_str()).c_str());
				ImGui::Text(("FPS: " + psreadvalue.get(0).get(/*super secret file layout stuff*/"").to_str()).c_str());
				ImGui::Text(("Length In Frames: " + std::to_string(psreadvalue.get<picojson::array>().size())).c_str());
				ImGui::End();
			}
			ImGui::SameLine();
			if (ImGui::Button("Close"))
			{
				isrecordopen = false;
				playing = false;
				psreaddata = "";
				psreadvalue = picojson::value();
				Window::fc = 1;
				Window::posX = 0;
				Window::posY = 0;
				Window::posZ = 0;
			}


			ImGui::Text(("X: " + std::to_string(posX)).c_str());
			ImGui::Text(("Y: " + std::to_string(posY)).c_str());
			ImGui::Text(("Z: " + std::to_string(posZ)).c_str());

			ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - border_image_width) * 0.5f, (ImGui::GetWindowSize().y - border_image_height) * 0.5f));
			ImGui::Image((void*)(intptr_t)border_image_texture, ImVec2(border_image_width, border_image_height));

			ImGui::SetCursorPos(ImVec2(((ImGui::GetWindowSize().x - player_image_width) * 0.5f) - (posX * multiplier), ((ImGui::GetWindowSize().y - player_image_height) * 0.5f) - (posZ * multiplier)));
			ImGui::Image((void*)(intptr_t)player_image_texture, ImVec2(player_image_width, player_image_height));

			if (playing)
			{
				if (Window::fc < psreadvalue.get<picojson::array>().size() - 1)
				{
					posX = stof(psreadvalue.get(Window::fc).get(/*super secret file layout stuff*/"").to_str());
					posY = stof(psreadvalue.get(Window::fc).get(/*super secret file layout stuff*/"").to_str());
					posZ = stof(psreadvalue.get(Window::fc).get(/*super secret file layout stuff*/"").to_str());

					fc += 1;
					std::this_thread::sleep_for(std::chrono::milliseconds(framewait));
				}
				else if (Window::fc == psreadvalue.get<picojson::array>().size() - 1)
				{
					playing = false;
					Window::fc = 1;
				}
			}
			
			
		}
		ImGui::End();
	}

	//help menu related stuff here vvv
	if (showFAQ)
	{
		ImGui::Begin("Help", &showFAQ, ImGuiWindowFlags_NoResize);
		ImGui::SetWindowSize(ImVec2(389,422));
		bool istableopen = ImGui::BeginTable("FAQ", 1, ImGuiTableFlags_Borders);
		ImGui::TableNextColumn();
		ImGui::Text("FAQ");		ImGui::TableNextColumn();
		ImGui::TextColored(ImVec4(231.0f / 255.0f, 76.0f / 255.0f, 60.0f / 255.0f, 1), "Where are some of my utilities?");
		ImGui::TextColored(ImVec4(46.0f / 255.0f, 204.0f / 255.0f, 113.0f / 255.0f, 1), "Fret not, most utilities including ping graphs, player\n data and other server related data will show up once\n you have entered a game!");
		ImGui::TableNextColumn();
		ImGui::TextColored(ImVec4(231.0f / 255.0f, 76.0f / 255.0f, 60.0f / 255.0f, 1), "Why wont my application close after pressing the X button?");
		ImGui::TextColored(ImVec4(46.0f / 255.0f, 204.0f / 255.0f, 113.0f / 255.0f, 1), "Check to make sure that all subsidiaries of the main\n window have been dragged back into the main window.\nSometimes the main window may take a while to close,\n this is normal while it is joining threads.");
		ImGui::TableNextColumn();
		ImGui::TextColored(ImVec4(231.0f / 255.0f, 76.0f / 255.0f, 60.0f / 255.0f, 1), "What data is collected?");
		ImGui::TextColored(ImVec4(46.0f / 255.0f, 204.0f / 255.0f, 113.0f / 255.0f, 1), "No data is collected by our program. Our servers are only\n contacted at the start of the program to check for updates\n and while installing a new version.");
		ImGui::TableNextColumn();
		ImGui::TextColored(ImVec4(231.0f / 255.0f, 76.0f / 255.0f, 60.0f / 255.0f, 1), "Will full match recording be coming?");
		ImGui::TextColored(ImVec4(46.0f / 255.0f, 204.0f / 255.0f, 113.0f / 255.0f, 1), "Yes, full match recording is being worked on including\n ping graph recording, player data, and other info!");
		ImGui::TableNextColumn();
		ImGui::Text("Configuration");
		ImGui::TableNextColumn();		
		ImGui::TextColored(ImVec4(231.0f / 255.0f, 76.0f / 255.0f, 60.0f / 255.0f, 1), "How do I configure the custom TTS message?");
		ImGui::TextColored(ImVec4(46.0f / 255.0f, 204.0f / 255.0f, 113.0f / 255.0f, 1), "When configuring the TTS message, anything that is put\n into the textbox, that isnt encompassed in curly brackets\n {}, will be spoken exactly as put in. The available options\n for what can go in the curly brackets are:\n isp, country, region, city.\n An example is:");
		char example[] = "The city is {city}, the country is {country} and the isp is {isp}";
		ImGui::InputText("", example, IM_ARRAYSIZE(example), ImGuiInputTextFlags_ReadOnly);

		istableopen ? ImGui::EndTable() : void();
		ImGui::End();
	}

	if (showCheckForUpdates)
	{
		// If an update is available
		if (latestVersion > version)
		{
			ImGui::Begin("Update Menu", (bool*)0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking);

			ImGui::Text("New Update Available!");
			ImGui::Text(("Currently on Revision " + std::to_string(version)).c_str());
			ImGui::Text(("Latest is Revision " + std::to_string(latestVersion)).c_str());
			ImGui::Text(changelog.c_str());
			if (ImGui::Button("Update", ImVec2(100, 30)))
			{
				ShellExecuteA(NULL, "open", "autoupdate.exe", NULL, "..", SW_SHOW);
				exit(0);
			}
			ImGui::End();
		}
		else
		{
			ImGui::Begin("Update Menu", (bool*)0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking);
			ImGui::SetWindowFontScale(1.25);
			TextCenter("Up to date.");
			ImGui::SetWindowFontScale(1);
			ImGui::Text(("Currently on Revision " + std::to_string(version)).c_str());

			ImGui::Text(changelog.c_str());
			if (ImGui::Button("Close", ImVec2(100, 30)))
			{
				showCheckForUpdates = false;
			}
			ImGui::End();
		}
		
	}

	if (showJoinAtlas)
	{
		ImGui::Begin("Join Atlas Link", &showJoinAtlas);
		char joinlink[150];
		ImGui::InputText("", joinlink, IM_ARRAYSIZE(joinlink));
		ImGui::SameLine();
		if (ImGui::Button("Join"))
		{
			std::wstring a = tts::s2ws(std::string(joinlink));
			LPCWSTR result = a.c_str();
			ShellExecute(0, 0, result, 0, 0, SW_SHOW);
			failed = false;
		}
		ImGui::End();
	}

	if (showSettings)
	{
		ImGui::Begin("Settings", &showSettings, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking);

		ImGui::SetWindowSize(ImVec2(256, 512));

		bool tableopen(ImGui::BeginTable("settings", 1, ImGuiTableFlags_Borders));

		ImGui::TableNextColumn();

		ImGui::Text("TTS");

		ImGui::TableNextColumn();

		ImGui::Checkbox("TTS", &isTTS);
		if(isTTS)
		{
			ImGui::Checkbox("Custom Join Server Message", &customJoinMessage);
			if(customJoinMessage)
			{
				ImGui::InputText("Message", customMessage, IM_ARRAYSIZE(customMessage));
				ImGui::Text("Custom Message Keywords:");
				ImGui::Text("{city}\n{region} (state or province)\n{country}\n{isp}");
			}
		}

		ImGui::TableNextColumn();

		ImGui::Text("General");

		ImGui::TableNextColumn();

		ImGui::Checkbox("Show Ping Graphs", &showPingGraph);

		ImGui::Checkbox("Show Match Info on Discord", &useDiscordRP);

		ImGui::Checkbox("Beep Near Playspace Boundary", &enableBeep);

		ImGui::Checkbox("Playspace Greenscreen Mode", &isgreenscreen);

		ImGui::InputText("Headset IP", s_headsetIP, IM_ARRAYSIZE(s_headsetIP));
		
		ImGui::TableNextColumn();

		ImGui::Text("Application");

		ImGui::TableNextColumn();

		const char* items[] = { "Grey", "Cherry", "Dark", "ImGui Dark", "Source", "Gruvbox" };
		static const char* current_item = theme.c_str();

		if (ImGui::BeginCombo("Theme", current_item)) // The second parameter is the label previewed before opening the combo.
		{
			for (int n = 0; n < IM_ARRAYSIZE(items); n++)
			{
				bool is_selected = (current_item == items[n]); // You can store your selection however you want, outside or inside your objects
				if (ImGui::Selectable(items[n], is_selected))
				{
					current_item = items[n];
					theme = std::string(current_item);
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
			}
			ImGui::EndCombo();
		}

		tableopen ? ImGui::EndTable() : void();
		ImGui::SetWindowFontScale(.75);
		if (ImGui::Button("Save", ImVec2(50, 25)))
		{
			SettingsManager::CreateNewSettingsBool("enableBeep", enableBeep);
			SettingsManager::CreateNewSettingsBool("useDiscordRP", useDiscordRP);
			SettingsManager::CreateNewSettingsBool("showPingGraph", showPingGraph);
			SettingsManager::CreateNewSettingsBool("isTTS", isTTS);
			SettingsManager::CreateNewSettingsString("customMessage", customMessage);
			SettingsManager::CreateNewSettingsBool("customJoinMessage", customJoinMessage);
			SettingsManager::CreateNewSettingsString("headsetIP", std::string(s_headsetIP, sizeof(s_headsetIP) / sizeof(char)));
			if (current_item)
				SettingsManager::CreateNewSettingsString("theme", std::string(current_item));
			SettingsManager::WriteSettingsToFile();

			UpdateTheme();
		}
		ImGui::SetWindowFontScale(1);


		ImGui::End();
	}


	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	ImGui::UpdatePlatformWindows();
	ImGui::RenderPlatformWindowsDefault();
	SDL_GL_MakeCurrent(m_window, m_gl_context);

	SDL_GL_SwapWindow(m_window);

	float endtime = SDL_GetTicks() / 1000.0f;

	float deltatime = endtime - starttime;
	float targetsleep = 1.0f/60;
	if(deltatime < targetsleep)
	std::this_thread::sleep_for(std::chrono::milliseconds(int((targetsleep - deltatime) * 1000)));
}

void Window::UpdateTheme()
{
	CherryTheme();
	if (theme == "Cherry")
		CherryTheme();
	else if (theme == "Grey")
		GreyTheme();
	else if (theme == "Dark")
		DarkTheme();
	else if (theme == "ImGui Dark")
		ImGui::StyleColorsDark();
	else if (theme == "Source")
		SourceTheme();
	else if (theme == "Gruvbox")
		GruvboxTheme();
}

void Window::DarkTheme()
{
	ImGui::StyleColorsDark();
	auto& style = ImGui::GetStyle();

	style.WindowRounding = 0.0f;             // Radius of window corners rounding. Set to 0.0f to have rectangular windows
	style.ScrollbarRounding = 0.0f;             // Radius of grab corners rounding for scrollbar
	style.GrabRounding = 0.0f;             // Radius of grabs corners rounding. Set to 0.0f to have rectangular slider grabs.
	style.AntiAliasedLines = true;
	style.AntiAliasedFill = true;
	style.WindowRounding = 0;
	style.ChildRounding = 0;
	style.ScrollbarSize = 16;
	style.ScrollbarRounding = 2;
	style.GrabRounding = 0;
	style.ItemSpacing.x = 10;
	style.ItemSpacing.y = 4;
	style.IndentSpacing = 22;
	style.FramePadding.x = 6;
	style.FramePadding.y = 4;
	style.Alpha = 1.0f;
	style.FrameRounding = 0.0f;

	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
	colors[ImGuiCol_Border] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, 0.65f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.09f, 0.21f, 0.31f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.29f, 0.55f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}
void Window::CherryTheme() {
	ImGui::StyleColorsDark();
	// cherry colors, 3 intensities
#define HI(v)   ImVec4(0.502f, 0.075f, 0.256f, v)
#define MED(v)  ImVec4(0.455f, 0.198f, 0.301f, v)
#define LOW(v)  ImVec4(0.232f, 0.201f, 0.271f, v)
// backgrounds (@todo: complete with BG_MED, BG_LOW)
#define BG(v)   ImVec4(0.200f, 0.220f, 0.270f, v)
// text
#define TEXT(v) ImVec4(0.860f, 0.930f, 0.890f, v)

	auto& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_Text] = TEXT(0.78f);
	style.Colors[ImGuiCol_TextDisabled] = TEXT(0.28f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
	style.Colors[ImGuiCol_PopupBg] = BG(0.9f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.31f, 0.31f, 1.00f, 0.00f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style.Colors[ImGuiCol_FrameBg] = BG(1.00f);
	style.Colors[ImGuiCol_FrameBgHovered] = MED(0.78f);
	style.Colors[ImGuiCol_FrameBgActive] = MED(1.00f);
	style.Colors[ImGuiCol_TitleBg] = LOW(1.00f);
	style.Colors[ImGuiCol_TitleBgActive] = HI(1.00f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = BG(0.75f);
	style.Colors[ImGuiCol_MenuBarBg] = BG(0.47f);
	style.Colors[ImGuiCol_ScrollbarBg] = BG(1.00f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.09f, 0.15f, 0.16f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = MED(0.78f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = MED(1.00f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
	style.Colors[ImGuiCol_ButtonHovered] = MED(0.86f);
	style.Colors[ImGuiCol_ButtonActive] = MED(1.00f);
	style.Colors[ImGuiCol_Header] = MED(0.76f);
	style.Colors[ImGuiCol_HeaderHovered] = MED(0.86f);
	style.Colors[ImGuiCol_HeaderActive] = HI(1.00f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
	style.Colors[ImGuiCol_ResizeGripHovered] = MED(0.78f);
	style.Colors[ImGuiCol_ResizeGripActive] = MED(1.00f);
	style.Colors[ImGuiCol_PlotLines] = TEXT(0.63f);
	style.Colors[ImGuiCol_PlotLinesHovered] = MED(1.00f);
	style.Colors[ImGuiCol_PlotHistogram] = TEXT(0.63f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = MED(1.00f);
	style.Colors[ImGuiCol_TextSelectedBg] = MED(0.43f);
	// [...]

	style.WindowPadding = ImVec2(6, 4);
	style.WindowRounding = 0.0f;
	style.FramePadding = ImVec2(5, 2);
	style.FrameRounding = 3.0f;
	style.ItemSpacing = ImVec2(7, 1);
	style.ItemInnerSpacing = ImVec2(1, 1);
	style.TouchExtraPadding = ImVec2(0, 0);
	style.IndentSpacing = 6.0f;
	style.ScrollbarSize = 12.0f;
	style.ScrollbarRounding = 16.0f;
	style.GrabMinSize = 20.0f;
	style.GrabRounding = 2.0f;

	style.Colors[ImGuiCol_Border] = ImVec4(0.539f, 0.479f, 0.255f, 0.162f);
	style.FrameBorderSize = 0.0f;
	style.WindowBorderSize = 1.0f;
}

void Window::GreyTheme()
{
	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	/// 0 = FLAT APPEARENCE
	/// 1 = MORE "3D" LOOK
	int is3D = 0;

	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.12f, 0.71f);
	colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.42f, 0.42f, 0.42f, 0.54f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.67f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.335f, 0.335f, 0.335f, 1.000f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.24f, 0.24f, 0.24f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.54f, 0.54f, 0.54f, 0.35f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.52f, 0.52f, 0.52f, 0.59f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.76f, 0.76f, 0.76f, 0.77f);
	colors[ImGuiCol_Separator] = ImVec4(0.000f, 0.000f, 0.000f, 0.137f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.700f, 0.671f, 0.600f, 0.290f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.702f, 0.671f, 0.600f, 0.674f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);

	style.PopupRounding = 3;

	style.WindowPadding = ImVec2(4, 4);
	style.FramePadding = ImVec2(6, 4);
	style.ItemSpacing = ImVec2(6, 2);

	style.ScrollbarSize = 18;

	style.WindowBorderSize = 0;
	style.ChildBorderSize = 0;
	style.PopupBorderSize = 0;
	style.FrameBorderSize = is3D;

	style.WindowRounding = 3;
	style.ChildRounding = 3;
	style.FrameRounding = 3;
	style.ScrollbarRounding = 2;
	style.GrabRounding = 3;

#ifdef IMGUI_HAS_DOCK 
	style.TabBorderSize = is3D;
	style.TabRounding = 3;

	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
	colors[ImGuiCol_Tab] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
	colors[ImGuiCol_DockingPreview] = ImVec4(0.85f, 0.85f, 0.85f, 0.28f);

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}
#endif
}

void Window::SourceTheme()
{
	ImGui::StyleColorsDark();
	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.29f, 0.34f, 0.26f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.29f, 0.34f, 0.26f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.14f, 0.16f, 0.11f, 0.52f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.27f, 0.30f, 0.23f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.34f, 0.26f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.29f, 0.34f, 0.26f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.28f, 0.32f, 0.24f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.25f, 0.30f, 0.22f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.23f, 0.27f, 0.21f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
	colors[ImGuiCol_Button] = ImVec4(0.29f, 0.34f, 0.26f, 0.40f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
	colors[ImGuiCol_Header] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.42f, 0.31f, 0.6f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
	colors[ImGuiCol_Separator] = ImVec4(0.14f, 0.16f, 0.11f, 1.00f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.54f, 0.57f, 0.51f, 1.00f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.19f, 0.23f, 0.18f, 0.00f); // grip invis
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.54f, 0.57f, 0.51f, 1.00f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
	colors[ImGuiCol_Tab] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.54f, 0.57f, 0.51f, 0.78f);
	colors[ImGuiCol_TabActive] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
	colors[ImGuiCol_DockingPreview] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.78f, 0.28f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(0.73f, 0.67f, 0.24f, 1.00f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameBorderSize = 1.0f;
	style.WindowRounding = 0.0f;
	style.ChildRounding = 0.0f;
	style.FrameRounding = 0.0f;
	style.PopupRounding = 0.0f;
	style.ScrollbarRounding = 0.0f;
	style.GrabRounding = 0.0f;
	style.TabRounding = 0.0f;
}

void Window::GruvboxTheme()
{
	auto& style = ImGui::GetStyle();
	style.ChildRounding = 0;
	style.GrabRounding = 0;
	style.FrameRounding = 0;
	style.PopupRounding = 0;
	style.ScrollbarRounding = 0;
	style.TabRounding = 0;
	style.WindowRounding = 0;
	style.FramePadding = { 4, 4 };

	style.WindowTitleAlign = { 0.5, 0.5 };

	ImVec4* colors = ImGui::GetStyle().Colors;
	// Updated to use IM_COL32 for more precise colors and to add table colors (1.80 feature)
	colors[ImGuiCol_Text] = ImColor{ IM_COL32(0xeb, 0xdb, 0xb2, 0xFF) };
	colors[ImGuiCol_TextDisabled] = ImColor{ IM_COL32(0x92, 0x83, 0x74, 0xFF) };
	colors[ImGuiCol_WindowBg] = ImColor{ IM_COL32(0x1d, 0x20, 0x21, 0xF0) };
	colors[ImGuiCol_ChildBg] = ImColor{ IM_COL32(0x1d, 0x20, 0x21, 0xFF) };
	colors[ImGuiCol_PopupBg] = ImColor{ IM_COL32(0x1d, 0x20, 0x21, 0xF0) };
	colors[ImGuiCol_Border] = ImColor{ IM_COL32(0x1d, 0x20, 0x21, 0xFF) };
	colors[ImGuiCol_BorderShadow] = ImColor{ 0 };
	colors[ImGuiCol_FrameBg] = ImColor{ IM_COL32(0x3c, 0x38, 0x36, 0x90) };
	colors[ImGuiCol_FrameBgHovered] = ImColor{ IM_COL32(0x50, 0x49, 0x45, 0xFF) };
	colors[ImGuiCol_FrameBgActive] = ImColor{ IM_COL32(0x66, 0x5c, 0x54, 0xA8) };
	colors[ImGuiCol_TitleBg] = ImColor{ IM_COL32(0xd6, 0x5d, 0x0e, 0xFF) };
	colors[ImGuiCol_TitleBgActive] = ImColor{ IM_COL32(0xfe, 0x80, 0x19, 0xFF) };
	colors[ImGuiCol_TitleBgCollapsed] = ImColor{ IM_COL32(0xd6, 0x5d, 0x0e, 0x9C) };
	colors[ImGuiCol_MenuBarBg] = ImColor{ IM_COL32(0x28, 0x28, 0x28, 0xF0) };
	colors[ImGuiCol_ScrollbarBg] = ImColor{ IM_COL32(0x00, 0x00, 0x00, 0x28) };
	colors[ImGuiCol_ScrollbarGrab] = ImColor{ IM_COL32(0x3c, 0x38, 0x36, 0xFF) };
	colors[ImGuiCol_ScrollbarGrabHovered] = ImColor{ IM_COL32(0x50, 0x49, 0x45, 0xFF) };
	colors[ImGuiCol_ScrollbarGrabActive] = ImColor{ IM_COL32(0x66, 0x5c, 0x54, 0xFF) };
	colors[ImGuiCol_CheckMark] = ImColor{ IM_COL32(0xd6, 0x5d, 0x0e, 0x9E) };
	colors[ImGuiCol_SliderGrab] = ImColor{ IM_COL32(0xd6, 0x5d, 0x0e, 0x70) };
	colors[ImGuiCol_SliderGrabActive] = ImColor{ IM_COL32(0xfe, 0x80, 0x19, 0xFF) };
	colors[ImGuiCol_Button] = ImColor{ IM_COL32(0xd6, 0x5d, 0x0e, 0x66) };
	colors[ImGuiCol_ButtonHovered] = ImColor{ IM_COL32(0xfe, 0x80, 0x19, 0x9E) };
	colors[ImGuiCol_ButtonActive] = ImColor{ IM_COL32(0xfe, 0x80, 0x19, 0xFF) };
	colors[ImGuiCol_Header] = ImColor{ IM_COL32(0xd6, 0x5d, 0x0e, 0.4F) };
	colors[ImGuiCol_HeaderHovered] = ImColor{ IM_COL32(0xfe, 0x80, 0x19, 0xCC) };
	colors[ImGuiCol_HeaderActive] = ImColor{ IM_COL32(0xfe, 0x80, 0x19, 0xFF) };
	colors[ImGuiCol_Separator] = ImColor{ IM_COL32(0x66, 0x5c, 0x54, 0.50f) };
	colors[ImGuiCol_SeparatorHovered] = ImColor{ IM_COL32(0x50, 0x49, 0x45, 0.78f) };
	colors[ImGuiCol_SeparatorActive] = ImColor{ IM_COL32(0x66, 0x5c, 0x54, 0xFF) };
	colors[ImGuiCol_ResizeGrip] = ImColor{ IM_COL32(0xd6, 0x5d, 0x0e, 0x40) };
	colors[ImGuiCol_ResizeGripHovered] = ImColor{ IM_COL32(0xfe, 0x80, 0x19, 0xAA) };
	colors[ImGuiCol_ResizeGripActive] = ImColor{ IM_COL32(0xfe, 0x80, 0x19, 0xF2) };
	colors[ImGuiCol_Tab] = ImColor{ IM_COL32(0xd6, 0x5d, 0x0e, 0xD8) };
	colors[ImGuiCol_TabHovered] = ImColor{ IM_COL32(0xfe, 0x80, 0x19, 0xCC) };
	colors[ImGuiCol_TabActive] = ImColor{ IM_COL32(0xfe, 0x80, 0x19, 0xFF) };
	colors[ImGuiCol_TabUnfocused] = ImColor{ IM_COL32(0x1d, 0x20, 0x21, 0.97f) };
	colors[ImGuiCol_TabUnfocusedActive] = ImColor{ IM_COL32(0x1d, 0x20, 0x21, 0xFF) };
	colors[ImGuiCol_PlotLines] = ImColor{ IM_COL32(0xd6, 0x5d, 0x0e, 0xFF) };
	colors[ImGuiCol_PlotLinesHovered] = ImColor{ IM_COL32(0xfe, 0x80, 0x19, 0xFF) };
	colors[ImGuiCol_PlotHistogram] = ImColor{ IM_COL32(0x98, 0x97, 0x1a, 0xFF) };
	colors[ImGuiCol_PlotHistogramHovered] = ImColor{ IM_COL32(0xb8, 0xbb, 0x26, 0xFF) };
	colors[ImGuiCol_TextSelectedBg] = ImColor{ IM_COL32(0x45, 0x85, 0x88, 0x59) };
	colors[ImGuiCol_DragDropTarget] = ImColor{ IM_COL32(0x98, 0x97, 0x1a, 0.90f) };
	colors[ImGuiCol_TableHeaderBg] = ImColor{ IM_COL32(0x38, 0x3c, 0x36, 0xFF) };
	colors[ImGuiCol_TableBorderStrong] = ImColor{ IM_COL32(0x28, 0x28, 0x28, 0xFF) };
	colors[ImGuiCol_TableBorderLight] = ImColor{ IM_COL32(0x38, 0x3c, 0x36, 0xFF) };
	colors[ImGuiCol_TableRowBg] = ImColor{ IM_COL32(0x1d, 0x20, 0x21, 0xFF) };
	colors[ImGuiCol_TableRowBgAlt] = ImColor{ IM_COL32(0x28, 0x28, 0x28, 0xFF) };
	colors[ImGuiCol_TextSelectedBg] = ImColor{ IM_COL32(0x45, 0x85, 0x88, 0xF0) };
	colors[ImGuiCol_NavHighlight] = ImColor{ IM_COL32(0x83, 0xa5, 0x98, 0xFF) };
	colors[ImGuiCol_NavWindowingHighlight] = ImColor{ IM_COL32(0xfb, 0xf1, 0xc7, 0xB2) };
	colors[ImGuiCol_NavWindowingDimBg] = ImColor{ IM_COL32(0x7c, 0x6f, 0x64, 0x33) };
	colors[ImGuiCol_ModalWindowDimBg] = ImColor{ IM_COL32(0x1d, 0x20, 0x21, 0x59) };
}