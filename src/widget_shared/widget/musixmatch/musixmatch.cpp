#include <QJsonDocument>

#include <base/logger_impl.h>
#include <base/logger.h>
#include <widget/util/str_util.h>
#include <widget/util/json_util.h>
#include <widget/musixmatch/musixmatch.h>

namespace {
	const auto kBaseUrl = "https://apic-desktop.musixmatch.com/ws/1.1/"_str;

    QString FormatTimeToLrc(double total_time) {
        int minutes = static_cast<int>(total_time) / 60;
        int seconds = static_cast<int>(total_time) % 60;
        int hundredths = static_cast<int>((total_time - static_cast<int>(total_time)) * 100);
        return qFormat("[%1:%2.%3]").arg(minutes, 2, 10, QChar(static_cast<ushort>('0')))
            .arg(seconds, 2, 10, QChar(static_cast<ushort>('0')))
            .arg(hundredths, 2, 10, QChar(static_cast<ushort>('0')));
    }
}

MusixmatchHttpService::MusixmatchHttpService()
	: http_client_(kBaseUrl) {
	http_client_.setHeader("User-Agent"_str,
		"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) "
		"Musixmatch/0.19.4 Chrome/58.0.3029.110 Electron/1.7.6 Safari/537.36 "_str);
}

QCoro::Task<QString> MusixmatchHttpService::getUserToken() {
    http_client_.setUrl(qFormat("%1token.get").arg(kBaseUrl));

    QUrlQuery params;
    params.addQueryItem("user_language"_str, "en"_str);
    params.addQueryItem("app_id"_str, "web-desktop-app-v1.0"_str);
    http_client_.setParams(params);

    http_client_.addCookie(QNetworkCookie(QByteArray("AWSELB"), QByteArray("0")));
    http_client_.addCookie(QNetworkCookie(QByteArray("AWSELBCORS"), QByteArray("0")));
    http_client_.addAcceptJsonHeader();

    auto content = co_await http_client_.get();

    QJsonDocument json_response;
    if (!json_util::deserialize(content, json_response)) {
        co_return QString();
    }

    auto json_object = json_response.object();
    auto status_code = json_object["message"_str].toObject()["header"_str].toObject()["status_code"_str].toInt();
    auto hint = json_object["message"_str].toObject()["header"_str].toObject()["hint"_str].toString();

    if (status_code == 401 && hint == "captcha"_str) {
        co_return "captcha"_str;
    }
    else if (status_code != 200) {
        co_return QString();
    }
    QString user_token = json_object["message"_str].toObject()["body"_str].toObject()["user_token"_str].toString();
    if (user_token == "UpgradeOnlyUpgradeOnlyUpgradeOnlyUpgradeOnly"_str) {
        co_return QString();
    }
    co_return user_token;
}

QCoro::Task<QString> MusixmatchHttpService::ensureUserToken() {
	if (user_token_.isEmpty()) {
		user_token_ = co_await getUserToken();
	}
	co_return user_token_;
}

QString MusixmatchHttpService::covertToLrcString(const QString & content) {
    QJsonDocument json_doc;
    if (!json_util::deserialize(content, json_doc)) {
        return {};
    }

    auto root_obj = json_doc.object();
    auto message_obj = root_obj["message"_str].toObject();
    auto body_obj = message_obj["body"_str].toObject();

    auto macro_calls_obj = body_obj["macro_calls"_str].toObject();
    if (!macro_calls_obj.contains("track.subtitles.get"_str)) {
        return {};
    }

    auto track_subtitles_get_obj = macro_calls_obj["track.subtitles.get"_str].toObject();
    auto message_subtitles_get_obj = track_subtitles_get_obj["message"_str].toObject();
    auto body_subtitles_get_obj = message_subtitles_get_obj["body"_str].toObject();
    auto subtitle_list = body_subtitles_get_obj["subtitle_list"_str].toArray();

    QString lrc_content;

    Q_FOREACH(const QJsonValue & value, subtitle_list) {
        auto subtitle_obj = value.toObject();
        auto subtitle = subtitle_obj["subtitle"_str].toObject();
        auto subtitle_body = subtitle["subtitle_body"_str].toString();
        auto subtitle_doc = QJsonDocument::fromJson(subtitle_body.toUtf8());
        if (subtitle_doc.isArray()) {
            QJsonArray subtitle_array = subtitle_doc.array();
            Q_FOREACH(const QJsonValue & item, subtitle_array) {
                auto subtitle_item = item.toObject();
                auto text = subtitle_item["text"_str].toString();
                double total_time = subtitle_item["time"_str].toObject()["total"_str].toDouble();
                auto lrc_time = FormatTimeToLrc(total_time);
                lrc_content += lrc_time + text + "\r\n"_str;
            }
        }
    }
    return lrc_content;
}

QCoro::Task<QString> MusixmatchHttpService::search(const QString& q_album, const QString& q_artist, const QString& q_title) {
    QUrlQuery params;
    params.addQueryItem("subtitle_format"_str, "mxm"_str);
    params.addQueryItem("namespace"_str, "lyrics_richsynched"_str);
    params.addQueryItem("app_id"_str, "web-desktop-app-v1.0"_str);
    params.addQueryItem("q_album"_str, q_album);
    params.addQueryItem("q_artist"_str, q_artist);
    params.addQueryItem("q_track"_str, q_title);
    params.addQueryItem("track_spotify_id"_str, kEmptyString);

	auto user_token = co_await ensureUserToken();
    params.addQueryItem("usertoken"_str, user_token);

    http_client_.setUrl(qFormat("%1macro.subtitles.get").arg(kBaseUrl));
    http_client_.setParams(params);

    auto content = co_await http_client_.get();
	auto lrc = covertToLrcString(content);
	if (lrc.isEmpty()) {
        XAMP_LOG_DEBUG("{}", content.toStdString());
	}
	co_return lrc;
}
