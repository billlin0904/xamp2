#include <widget/str_utilts.h>
#include <widget/discordnotify.h>

std::array<char, DicordNotify::kMaxDetailsLength> DicordNotify::details_;

DicordNotify::DicordNotify(QObject* parent)
	: QObject(parent) {
}

DicordNotify::~DicordNotify() {
	Discord_ClearPresence();
	Discord_Shutdown();
}

void DicordNotify::OnStateChanged(PlayerState play_state) {
	switch (play_state) {
	case PlayerState::PLAYER_STATE_PAUSED:
		discord_presence_.state = "Paused";
		discord_presence_.smallImageKey = "pause";
		break;
	case PlayerState::PLAYER_STATE_RESUME:
	case PlayerState::PLAYER_STATE_RUNNING:
		discord_presence_.state = "Listening";
		discord_presence_.smallImageKey = "play";
		break;
	case PlayerState::PLAYER_STATE_STOPPED:
		discord_presence_.state = "Stopped";
		discord_presence_.smallImageKey = "stop";
		break;
	default:
		return;
	}

	updateDiscordPresence();
}

void DicordNotify::OnNowPlaying(QString const& artist, QString const& title) {
	auto format = (artist + Q_UTF8(" - ") + title).toStdString();
	auto details_length = (std::min)(format.length(), details_.size() - 1);
	details_.fill(0);
	format = format.substr(0, details_length);
	strcpy_s(details_.data(), details_.size(), format.data());

	discord_presence_.state = "Listening";
	discord_presence_.smallImageKey = "xamp";
	discord_presence_.details = details_.data();
}

void DicordNotify::discordInit() {
	MemorySet(&handlers_, 0, sizeof(handlers_));
	handlers_.ready = callback_discord_connected;
	handlers_.disconnected = callback_discord_disconnected;
	handlers_.errored = callback_discord_errored;
	handlers_.joinGame = callback_discord_joingame;

	Discord_Initialize(kApplicationID, &handlers_, 1, nullptr);
	initDiscordPresence();
}

void DicordNotify::initDiscordPresence() {
	MemorySet(&discord_presence_, 0, sizeof(discord_presence_));
	discord_presence_.state = "Initialized";
	discord_presence_.details = "Waiting ...";
	discord_presence_.largeImageKey = "xamp";
	discord_presence_.smallImageKey = "xamp";

	updateDiscordPresence();
}

void DicordNotify::updateDiscordPresence() {
	Discord_UpdatePresence(&discord_presence_);

#ifdef DISCORD_DISABLE_IO_THREAD
	Discord_UpdateConnection();
#endif
	Discord_RunCallbacks();

	/*XAMP_LOG_DEBUG("Ran Discord presence update: {}, {}, {}, {}",
		discord_presence_.state,
		discord_presence_.details,
		discord_presence_.largeImageKey,
		discord_presence_.smallImageKey);*/
}

void DicordNotify::callback_discord_joingame(const char* joinSecret) {
	XAMP_LOG_DEBUG("{} is join game.", joinSecret);
}

void DicordNotify::callback_discord_connected(const DiscordUser* request) {
	XAMP_LOG_DEBUG("Connected to {}.", request->username);
}
void DicordNotify::callback_discord_disconnected(int errorCode, const char* message) {
	XAMP_LOG_DEBUG("Disconnected ({}): {}.", errorCode, message);
}

void DicordNotify::callback_discord_errored(int errorCode, const char* message) {
	XAMP_LOG_DEBUG("*** Error {}: {}.", errorCode, message);
}