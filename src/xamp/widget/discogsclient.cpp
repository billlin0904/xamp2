#include <QPixmap>

#include <base/logger.h>

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>

#include <widget/str_utilts.h>
#include <widget/discogsclient.h>

inline constexpr ConstLatin1String kDiscogsKey{ "TyvkLDUGhlzRhPkMdsmU" };
inline constexpr ConstLatin1String kDiscogsSeceret{ "oBilkJZyIDgEMkkGaQdVZEexZNRHUrVX" };
inline constexpr ConstLatin1String kDiscogsHost{"https://api.discogs.com"};

using namespace rapidjson;

template <typename TMemberIterator>
static QString toQString(TMemberIterator &value) {
    return QString::fromUtf8((*value).value.GetString(), (*value).value.GetStringLength());
}

DiscogsClient::DiscogsClient(QNetworkAccessManager* manager, QObject *parent)
    : QObject(parent)
    , manager_(manager) {
}

void DiscogsClient::downloadArtistImage(int32_t artist_id, const QString& url) {
    http::HttpClient(url, manager_)
        .download([=](auto data) {
        QPixmap image;
        image.loadFromData(data);
        if (image.isNull()) {
            return;
        }
        emit downloadImageFinished(artist_id, image);
        });
}

void DiscogsClient::searchArtistId(int32_t artist_id, const QString& id) {
    auto handler = [=](const QString& msg) {
        auto str = msg.toStdString();
        Document d;
        d.Parse(str.c_str());
        
        if (d.HasParseError()) {
            return;
        }

        auto images = d.FindMember("images");
        if (images == d.MemberEnd()) {
            XAMP_LOG_DEBUG("Not found image! id:{} artist id: {}", id.toStdString(), artist_id);
            return;
        }

        for (auto &image : (*images).value.GetArray()) {
            auto uri = image.FindMember("uri");
            if (uri == image.MemberEnd()) {
                return;
            }
            emit getArtistImageUrl(artist_id, toQString(uri));
            break;
        }
    };

    http::HttpClient(kDiscogsHost + Q_UTF8("/artists/") + id, manager_)
        .header(Q_UTF8("Authorization"), QString(Q_UTF8("Discogs key=%1, secret=%2")).arg(kDiscogsKey, kDiscogsSeceret))
        .success(handler)
        .get();
}

void DiscogsClient::searchArtist(int32_t artist_id, const QString &artist) {
    auto handler = [=](const QString& msg) {
        auto str = msg.toStdString();
        Document d;
        d.Parse(str.c_str());
        if (d.HasParseError()) {
            return;
        }
        auto results = d.FindMember("results");
        if (results == d.MemberEnd()) {
            return;
        }
        for (auto& result : (*results).value.GetArray()) {
            auto itr = result.FindMember("id");
            if (itr == result.MemberEnd()) {
                continue;
            }
            auto id = (*itr).value.GetInt();
            emit getArtistId(artist_id, QString::number(id));
            break;
        }
    };

    http::HttpClient(kDiscogsHost + Q_UTF8("/database/search"), manager_)
        .param(Q_UTF8("type"), Q_UTF8("release&artist"))
        .param(Q_UTF8("release_title"), artist)
        .param(Q_UTF8("artist"), artist)        
        .header(Q_UTF8("Authorization"), QString(Q_UTF8("Discogs key=%1, secret=%2")).arg(kDiscogsKey, kDiscogsSeceret))
        .success(handler)
        .get();
}
