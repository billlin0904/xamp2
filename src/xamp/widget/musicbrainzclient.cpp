#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>

#include <rapidxml.hpp>

#include <widget/database.h>
#include <player/chromaprinthelper.h>
#include <widget/str_utilts.h>
#include <widget/musicbrainzclient.h>

using namespace rapidjson;
using namespace rapidxml;

static constexpr ConstLatin1String kAcoustidKey{ "czKxnkyO" };
static constexpr ConstLatin1String kAcoustidHost { "https://api.acoustid.org/v2/lookup" };
static constexpr ConstLatin1String kMusicBrainzHost{ "http://musicbrainz.org/ws/2/" };

template <typename T>
static QString toQString(T itr) {
    return QString::fromUtf8(itr->value.GetString(), itr->value.GetStringLength());
}

MusicBrainzClient::MusicBrainzClient(QObject* parent)
	: QObject(parent) {
}

void MusicBrainzClient::searchBy(const FingerprintInfo& fingerprint) {
    auto handler = [this, fingerprint](const QString& msg) {
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

        QString artist_mbid;
        QString acoust_id;
        QList<QString> result_recordings;

        for (const auto& result : results->value.GetArray()) {
            auto id = result.FindMember("id");
            if (id != result.MemberEnd()) {
                acoust_id = toQString(id);
            }
            auto recordings = result.FindMember("recordings");
            if (recordings != result.MemberEnd()) {
                for (const auto& recording : recordings->value.GetArray()) {
                    auto releasegroups = recording.FindMember("releasegroups");
                    if (releasegroups != recording.MemberEnd()) {
                        for (const auto& release : releasegroups->value.GetArray()) {
                            auto title = release.FindMember("title");
                            if (title != release.MemberEnd()) {
                                qDebug() << "Found title:" << toQString(title);
                            }
                            auto id = release.FindMember("id");
                            if (id != release.MemberEnd()) {
                                qDebug() << "Found id:" << toQString(id);
                            }
                        }
                    }
                    if (recording.FindMember("artists") != recording.MemberEnd()) {
                        for (const auto& artist : recording["artists"].GetArray()) {
                            auto id = artist.FindMember("id");
                            if (id != artist.MemberEnd()) {
                                artist_mbid = toQString(id);
                                qDebug() << "Found artist mbid:" << artist_mbid;
                                lookupArtist(fingerprint.artist_id, artist_mbid);
                                return;
                            }
                        }
                    }
                }
            }
        }
    };

    http::HttpClient(kAcoustidHost)
        .param(Q_UTF8("client"), kAcoustidKey)
        .param(Q_UTF8("duration"), QString::number(fingerprint.duration))
        .param(Q_UTF8("meta"), Q_UTF8("recordings+releasegroups+compress"))
        .param(Q_UTF8("fingerprint"), fingerprint.fingerprint)
        .success(handler)
        .post();
}

void MusicBrainzClient::lookupArtist(int32_t artist_id, const QString& artist_mbid) {
    Database::instance().UpdateArtistMbid(artist_id, artist_mbid);

    auto handler = [this, artist_id](const QString& msg) {
        auto str = msg.toStdString();
        xml_document<> doc;
        doc.parse<0>(const_cast<char*>(str.data()));
        auto metadata = doc.first_node("metadata");
        if (!metadata) {
            return;
        }
        auto artist = metadata->first_node("artist");
        if (!artist) {
            return;
        }

        auto relation_list = artist->first_node("relation-list");
        if (!relation_list) {
            return;
        }

        std::string discogs_url;

        for (auto node = relation_list->first_node();
            node; node = node->next_sibling()) {
            auto itr = node->first_attribute("type");
            if (!itr) {
                continue;
            }
            std::string type(itr->value(), itr->value_size());
            if (type == "discogs") {
                auto target = node->first_node("target");
                std::string url(target->value(), target->value_size());
                discogs_url = url;
                break;
            }
        }

        auto pos = discogs_url.rfind('/');
        if (pos != std::string::npos) {
            auto id = discogs_url.substr(pos + 1);
            emit finished(artist_id, QString::fromStdString(id));
        }
    };

    http::HttpClient(kMusicBrainzHost + Q_UTF8("artist/") + artist_mbid)
        .param(Q_UTF8("inc"), Q_UTF8("url-rels"))
        .success(handler)
        .get();
}
