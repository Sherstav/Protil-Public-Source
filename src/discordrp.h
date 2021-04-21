#pragma once
#include "discord/discord.h"

class DiscordRP
{
public:
	DiscordRP();
	~DiscordRP();

	void RunCallbacks();
	void Update();
	void ClearActivity();

	void DeleteCore();
	void CreateCore();

	bool coreExists;

	discord::Activity m_activity;
	discord::Core* m_core;
};