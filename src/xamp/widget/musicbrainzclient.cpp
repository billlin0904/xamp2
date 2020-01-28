#include <rapidxml.hpp>

#include <widget/str_utilts.h>
#include <widget/musicbrainzclient.h>

using namespace rapidxml;

static bool parseXml(const QString &msg,
                     std::string_view child_name,
                     std::function<bool (xml_node<char> *)> &&callback) {
    auto str = msg.toStdString();

    xml_document<> doc;
    try {
        doc.parse<0>(const_cast<char *>(str.c_str()));
    } catch (...) {
        return false;
    }

    auto root = doc.first_node("metadata");
    if (!root) {
        return false;
    }

    auto child_node = root->first_node(child_name.data());
    if (!child_node) {
        return false;
    }

    for (auto node = child_node->first_node(); node != nullptr; node = node->next_sibling()) {
        if (!callback(node)) {
            return true;
        }
    }
    return false;
}

MusicBrainzClient::MusicBrainzClient(QObject *parent)
    : QObject(parent) {
}

void MusicBrainzClient::searchArtist(int32_t artist_id, const QString &artist) {
    auto handler = [=](const QString &message) {
        parseXml(message, "artist-list", [=](auto node) {
            std::string name = node->name();
            if (name == "artist") {
                std::string mbid = node->first_attribute("id")->value();
                emit onGetArtistMBID(artist_id, QString::fromStdString(mbid));
                return false;
            }
            return true;
        });
    };

    http::HttpClient(Q_UTF8("http://musicbrainz.org/ws/2/artist/"))
        .header(Q_UTF8("User-Agent"), Q_UTF8("xamp-player/1.0.0"))
        .param(Q_UTF8("query"), artist)
        .success(handler)
        .get();
}

void MusicBrainzClient::getArtistImageUrl(const QString &mbid) {
    auto handler = [this](const QString &message) {
        parseXml(message, "artist-list", [this](auto node) {
            std::string name = node->name();
            if (name == "relation") {
                std::string type = node->first_attribute("type")->value();
                if (type == "image") {
                    auto target = node->first_node("target");
                    std::string value = target->value();
                }
            }
            return true;
        });
    };

    http::HttpClient(Q_UTF8("http://musicbrainz.org/ws/2/artist/") + mbid)
        .header(Q_UTF8("User-Agent"), Q_UTF8("xamp-player/1.0.0"))
        .param(Q_UTF8("inc"), Q_UTF8("url-rels"))
        .success(handler)
        .get();
}
