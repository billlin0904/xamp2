#include <QJsonDocument>
#include <QDesktopServices>
#include <QSaveFile>

#include <widget/youtubedl/ytmusicoauth.h>

inline constexpr auto kOAuthCodeUrl = qTEXT("https://www.youtube.com/o/oauth2/device/code");
inline constexpr auto kOAuthScope = qTEXT("https://www.googleapis.com/auth/youtube");

inline constexpr auto kOAuthTokenUrl = qTEXT("https://oauth2.googleapis.com/token");
inline constexpr auto kOAuthClientSecret = qTEXT("SboVhoG9s0rNafixCSGGKXAT");

inline constexpr auto kOAuthClientId =
qTEXT("861556708454-d6dlm3lh05idd8npek18k6be8ba3oc68.apps.googleusercontent.com");

inline constexpr auto kOAuthUserAgent =
qTEXT("Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:88.0) Gecko/20100101 Firefox/88.0 Cobalt/Version");

XAMP_DECLARE_LOG_NAME(YtMusicOAuth);

YtMusicOAuth::YtMusicOAuth() {
	logger_ = LoggerManager::GetInstance().GetLogger(kYtMusicOAuthLoggerName);
}

void YtMusicOAuth::setup() {
	http::HttpClient(kOAuthCodeUrl)
		.header(qTEXT("User-Agent"), kOAuthUserAgent)
		.param(qTEXT("client_id"), kOAuthClientId)
		.param(qTEXT("scope"), kOAuthScope)
		.success([this](const auto &url, const auto &content) {
		XAMP_LOG_DEBUG("{}", content.toStdString());

		QJsonParseError error;
		auto code = QJsonDocument::fromJson(content.toUtf8(), &error);
		if (error.error != QJsonParseError::NoError) {
			return;
		}

		user_code_ = code["user_code"].toString();
		verification_url_ = code["verification_url"].toString();
		device_code_ = code["device_code"].toString();

		auto open_url = qSTR("%1?user_code=%2").arg(verification_url_).arg(user_code_);
		QDesktopServices::openUrl(open_url);
		emit acceptAuthorization();
		})
		.post();
}

void YtMusicOAuth::requestGrant() {
	http::HttpClient(kOAuthTokenUrl)
		.header(qTEXT("User-Agent"), kOAuthUserAgent)
		.param(qTEXT("client_secret"), kOAuthClientSecret)
		.param(qTEXT("grant_type"), qTEXT("urn:ietf:params:oauth:grant-type:device_code"))
		.param(qTEXT("device_code"), device_code_)
		.param(qTEXT("client_id"), kOAuthClientId)
		.success([this](const auto& url, const auto& content) {
		XAMP_LOG_DEBUG("{}", content.toStdString());
		QSaveFile file("oauth.json");
		file.open(QIODevice::WriteOnly);
		file.write(content.toUtf8());
		if (file.commit()) {
			emit requestGrantCompleted();
		}
	})
	.post();
}
