#include <QFuture>
#include <QtConcurrent>

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>

#include <rapidxml.hpp>

#include <widget/database.h>
#include <player/chromaprinthelper.h>
#include <widget/str_utilts.h>
#include <widget/musicbrainzclient.h>

using namespace rapidjson;
using namespace rapidxml;

static const QLatin1String ACOUSTID_KEY{ "czKxnkyO" };
static const QLatin1String ACOUSTID_HOST { "https://api.acoustid.org/v2/lookup" };
static const QLatin1String MUSICBRAINZ_HOST{ "http://musicbrainz.org/ws/2/" };

template <typename T>
static QString toQString(T itr) {
    return QString::fromUtf8(itr->value.GetString(), itr->value.GetStringLength());
}

MusicBrainzClient::MusicBrainzClient(QObject* parent)
	: QObject(parent)
    , fingerprint_watcher_(nullptr) {
}

void MusicBrainzClient::readFingerprint(int32_t artist_id, const QString& file_path) {
    file_paths_.append(file_path);

    std::function<Fingerprint(const QString&)> handler([artist_id](const QString& file_path) {
        QByteArray fingerprint;
        Fingerprint info;
        try {
            auto result = xamp::player::ReadFingerprint(file_path.toStdWString());
            fingerprint.append(reinterpret_cast<char*>(result.fingerprint.data()), result.fingerprint.size());
            info.artist_id = artist_id;
            info.duration = std::rint(result.duration);
            info.fingerprint = QString::fromLatin1(fingerprint.data(), fingerprint.size());
        }
        catch (const std::exception & e) {
            XAMP_LOG_DEBUG("ReadFingerprint return failure!, {}", e.what());           
        }        
        return info;
    });

    auto future = QtConcurrent::mapped(file_paths_, handler);
    fingerprint_watcher_ = new QFutureWatcher<Fingerprint>(this);
    fingerprint_watcher_->setFuture(future);

    (void) QObject::connect(fingerprint_watcher_,
        &QFutureWatcher<QString>::resultReadyAt,
        this,
        &MusicBrainzClient::fingerprintFound);
}

void MusicBrainzClient::cancel() {
    if (fingerprint_watcher_ != nullptr) {
        fingerprint_watcher_->cancel();
        delete fingerprint_watcher_;
        fingerprint_watcher_ = nullptr;
    }
}

void MusicBrainzClient::fingerprintFound(int index) {
    auto watcher = static_cast<QFutureWatcher<Fingerprint>*>(sender());
    if (!watcher || index >= file_paths_.count()) {
        return;
    }

    file_paths_.removeAt(index);
    const auto f = watcher->resultAt(index);
    qDebug() << "Read fingerprint completed artist id:" << f.artist_id;

    auto handler = [this, f](const QString& msg) {
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

        for (const auto & result : results->value.GetArray()) {            
            auto id = result.FindMember("id");
            if (id != result.MemberEnd()) {
                acoust_id = toQString(id);
            }
            auto recordings = result.FindMember("recordings");
            if (recordings != result.MemberEnd()) {
                for (const auto& recording : recordings->value.GetArray()) {
                    if (recording.FindMember("artists") == recording.MemberEnd()) {
                        continue;
                    }
                    for (const auto& artist : recording["artists"].GetArray()) {
                        auto id = artist.FindMember("id");
                        if (id != artist.MemberEnd()) {
                            artist_mbid = toQString(id);
                            qDebug() << "Found artist mbid:" << artist_mbid;
                            lookupArtist(f.artist_id, artist_mbid);
                            return;
                        }
                    }                    
                }
            }
        }
    };

    http::HttpClient(ACOUSTID_HOST)
        .param(Q_UTF8("client"), ACOUSTID_KEY)
        .param(Q_UTF8("duration"), QString::number(f.duration))
        .param(Q_UTF8("meta"), Q_UTF8("recordings+releasegroups+compress"))
        .param(Q_UTF8("fingerprint"), f.fingerprint)
        .success(handler)
        .post();
}

void MusicBrainzClient::lookupArtist(int32_t artist_id, const QString& artist_mbid) {
    Database::Instance().updateArtistMbid(artist_id, artist_mbid);

    auto handler = [this, artist_id](const QString& msg) {
        auto str = msg.toStdString();
        xml_document<> doc;
        doc.parse<0>((char*)str.data());
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

    http::HttpClient(MUSICBRAINZ_HOST + Q_UTF8("artist/") + artist_mbid)
        .param(Q_UTF8("inc"), Q_UTF8("url-rels"))
        .success(handler)
        .get();
}