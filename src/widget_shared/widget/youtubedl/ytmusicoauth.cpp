#include <QJsonDocument>
#include <QDesktopServices>
#include <QSaveFile>
#include <QDateTime>
#include <QJsonObject>
#include <QFile>

#include <widget/youtubedl/ytmusicoauth.h>

inline constexpr auto kOAuthCodeUrl = "https://www.youtube.com/o/oauth2/device/code"_str;
inline constexpr auto kOAuthYouTubeScope = "https://www.googleapis.com/auth/youtube"_str;

inline constexpr auto kOAuthTokenUrl = "https://oauth2.googleapis.com/token"_str;
inline constexpr auto kOAuthClientSecret = "SboVhoG9s0rNafixCSGGKXAT"_str;

inline constexpr auto kOAuthClientId =
"861556708454-d6dlm3lh05idd8npek18k6be8ba3oc68.apps.googleusercontent.com"_str;

inline constexpr auto kOAuthUserAgent =
"Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:88.0) Gecko/20100101 Firefox/88.0 Cobalt/Version"_str;

inline constexpr auto kGrantType = "urn:ietf:params:oauth:grant-type:device_code"_str;

XAMP_DECLARE_LOG_NAME(YtMusicOAuth);

YtMusicOAuth::YtMusicOAuth() {
	logger_ = XampLoggerFactory.GetLogger(XAMP_LOG_NAME(YtMusicOAuth));
}

void YtMusicOAuth::setup() {
	/*http::HttpClient(kOAuthCodeUrl)
		.header("User-Agent"_str, kOAuthUserAgent)
		.param("client_id"_str, kOAuthClientId)
		.param("scope"_str, kOAuthYouTubeScope)
		.error([this](const auto& url, const auto& content) {
			emit setupError();
		})
		.success([this](const auto &url, const auto &content) {
		XAMP_LOG_DEBUG("{}", content.toStdString());

		QJsonParseError error;
		auto code = QJsonDocument::fromJson(content.toUtf8(), &error);
		if (error.error != QJsonParseError::NoError) {
			emit setupError();
			return;
		}

		user_code_ = code["user_code"_str].toString();
		verification_url_ = code["verification_url"_str].toString();
		device_code_ = code["device_code"_str].toString();

		const auto open_url = qFormat("%1?user_code=%2").arg(verification_url_).arg(user_code_);
		emit acceptAuthorization(open_url);
		})
		.post();*/
}

void YtMusicOAuth::requestGrant() {
	/*http::HttpClient(kOAuthTokenUrl)
		.header("User-Agent"_str, kOAuthUserAgent)
		.param("client_secret"_str, kOAuthClientSecret)
		.param("grant_type"_str, kGrantType)
		.param("device_code"_str, device_code_)
		.param("client_id"_str, kOAuthClientId)
		.error([this](const auto& url, const auto& content) {
			emit requestGrantError();
		})
		.success([this](const auto& url, const auto& content) {
		XAMP_LOG_DEBUG("{}", content.toStdString());

		QJsonParseError error;
		auto code = QJsonDocument::fromJson(content.toUtf8(), &error);
		if (error.error != QJsonParseError::NoError) {
			emit requestGrantError();
			return;
		}
		auto root = code.object();
		root["expires_at"_str] = QDateTime::currentSecsSinceEpoch() + code["expires_in"_str].toInt();
		code.setObject(root);
		auto json = code.toJson();

		QSaveFile file("oauth.json"_str);
		file.open(QIODevice::WriteOnly);
		file.write(json);
		if (file.commit()) {
			emit requestGrantCompleted();
		} else {
			emit requestGrantError();
		}
	})
	.post();*/
}

std::optional<OAuthToken> YtMusicOAuth::parseOAuthJson() {
	QFile file("oauth.json"_str);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return std::nullopt;
	}

	const auto content = file.readAll();
	QJsonParseError error;
	const auto code = QJsonDocument::fromJson(content, &error);
	if (error.error != QJsonParseError::NoError) {
        return std::nullopt;
	}

	const auto root = code.object();
	const QList<QString> keys{
		"access_token"_str,
		"expires_in"_str,
		"refresh_token"_str,
		"scope"_str,
		"token_type"_str,
		"expires_at"_str,
	};

	OAuthToken token;
	Q_FOREACH(auto key, keys) {
		if (!root.contains(key)) {
            return std::nullopt;
		}
	}

	token.expires_in    = root["expires_in"_str].toInteger();
	token.expires_at    = root["expires_at"_str].toInteger();
	token.refresh_token = root["refresh_token"_str].toString();
	token.access_token  = root["access_token"_str].toString();
	token.token_type    = root["token_type"_str].toString();
	token.scope         = root["scope"_str].toString();

    return token;
}
