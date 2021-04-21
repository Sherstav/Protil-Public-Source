#include "discordrp.h"
#include <string>
#include <iostream>
#include <ctime>


DiscordRP::DiscordRP()
{
	auto result = discord::Core::Create(/*discord client id*/0, DiscordCreateFlags_Default, &m_core);
	m_activity.SetType(discord::ActivityType::Playing);
	m_activity.GetAssets().SetLargeImage("echovr");
	coreExists = true;
}

DiscordRP::~DiscordRP()
{

}

void DiscordRP::RunCallbacks()
{
	m_core->RunCallbacks();
}

void DiscordRP::Update()
{
	m_core->ActivityManager().UpdateActivity(m_activity, [](discord::Result result) {
		});
}

void DiscordRP::ClearActivity()
{
	m_core->ActivityManager().ClearActivity([](discord::Result result)
	{
		if (result == discord::Result::Ok)
		{
			std::cout << "Success clearing activity" << std::endl;
		}
		else
		{
			std::cout << "Failed clearing activity" << std::endl;
		}
	});
}

void DiscordRP::DeleteCore()
{
	m_core->~Core();
	coreExists = false;
	std::cout << "Deleted core" << std::endl;
}

void DiscordRP::CreateCore()
{
	auto result = discord::Core::Create(/*discord client id*/0, DiscordCreateFlags_Default, &m_core);
	m_activity.SetType(discord::ActivityType::Playing);
	m_activity.GetAssets().SetLargeImage("echovr");
	coreExists = true;
	std::cout << "Created core" << std::endl;
}
