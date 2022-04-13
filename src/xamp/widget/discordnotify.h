//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <discord_rpc.h>

#include <base/memory.h>
#include <base/logger.h>
#include <player/playstate.h>

using xamp::player::PlayerState;

class DicordNotify : public QObject {
	Q_OBJECT
public:
	static constexpr size_t kMaxDetailsLength = 128;
	static constexpr char kApplicationID[] = "871239612742389760";

	explicit DicordNotify(QObject *parent = nullptr);

	virtual ~DicordNotify() override;

	void discordInit();
		
public slots:
	void OnStateChanged(PlayerState play_state);

	void OnNowPlaying(QString const &artist, QString const &title);

private:
	void initDiscordPresence();

	void updateDiscordPresence();

	static void callback_discord_joingame(const char* joinSecret);

	static void callback_discord_connected(const DiscordUser* request);

	static void callback_discord_disconnected(int errorCode, const char* message);

	static void callback_discord_errored(int errorCode, const char* message);

	DiscordEventHandlers handlers_;
	DiscordRichPresence discord_presence_;
	static std::array<char, kMaxDetailsLength> details_;
};