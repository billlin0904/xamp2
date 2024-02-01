#include <QJsonDocument>
#include <QDesktopServices>
#include <QOAuth2AuthorizationCodeFlow>

#include <widget/youtubedl/ytmusicoauth.h>

XAMP_DECLARE_LOG_NAME(YtMusicOAuth);

YtMusicOAuth::YtMusicOAuth() {
	logger_ = LoggerManager::GetInstance().GetLogger(kYtMusicOAuthLoggerName);
}

void YtMusicOAuth::setup(const QString& file_name) {
	/*auto* google = new QOAuth2AuthorizationCodeFlow(this);
	google->setScope(kOAuthScope);
	google->setAuthorizationUrl(authUri);
	google->setClientIdentifier(kOAuthClientId);
	google->setAccessTokenUrl(QUrl(kOAuthTokenUrl));
	google->setClientIdentifierSharedKey(kOAuthClientSecret);*/

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

		auto user_code = code["user_code"];
		auto verification_url = code["verification_url"];
		auto open_url = qSTR("%1?user_code=%2").arg(verification_url.toString()).arg(user_code.toString());
		auto device_code = code["device_code"];

		QDesktopServices::openUrl(open_url);

		http::HttpClient(kOAuthTokenUrl)
			.header(qTEXT("User-Agent"), kOAuthUserAgent)
			.param(qTEXT("client_secret"), kOAuthClientSecret)
			.param(qTEXT("grant_type"), qTEXT("http://oauth.net/grant_type/device/1.0"))
			.param(qTEXT("device_code"), device_code)
			.param(qTEXT("client_id"), kOAuthClientId)
			.success([](const auto& url, const auto& content) {
			XAMP_LOG_DEBUG("{}", content.toStdString());
			})
			.post();
		})
		.post();
}