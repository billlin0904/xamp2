/*
 * xamp-player/1.0.0
 * API Key: b35fa8521e57bf069d1c08ce99ce8493
 * Shared secret: 86375cf8a41c20a3ddd88ae50f7e3d54
 * billlin0904
 * */

#include <QPixmap>

#include <rapidxml.hpp>

#include <widget/str_utilts.h>
#include <widget/http.h>
#include <widget/lastfmclient.h>

static const QLatin1String LASTFM_API_HOST{"http://ws.audioscrobbler.com/2.0/"};
static const QLatin1String LASTFM_API_KEY{"b35fa8521e57bf069d1c08ce99ce8493"};

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

    auto lfm = doc.first_node("lfm");
    if (!lfm) {
        return false;
    }

    auto root = lfm->first_node("results");
    if (!root) {
        return false;
    }

    auto child_node = root->first_node(child_name.data());
    if (!child_node) {
        return false;
    }

    for (auto node = child_node->first_node();
         node != nullptr;
         node = node->next_sibling()) {
        if (!callback(node)) {
            return true;
        }
    }
    return false;
}

LastfmClient::LastfmClient(QObject *parent)
    : QObject(parent) {
}

void LastfmClient::searchArtist(int32_t artist_id, const QString &artist) {
    auto handler = [=](const QString &message) {
        parseXml(message, "artistmatches", [=](auto artistmatches) {
            for (auto node = artistmatches->first_node();
                 node != nullptr;
                 node = node->next_sibling()) {
                std::string name = node->name();
                if (name == "image") {
                    std::string value = node->value();
                    emit onGetArtistImageUrl(artist_id, QString::fromStdString(value));
                    return false;
                }
            }
            return true;
        });
    };

    http::HttpClient(LASTFM_API_HOST)
        .param(Q_UTF8("method"), Q_UTF8("artist.search"))
        .param(Q_UTF8("artist"), artist)
        .param(Q_UTF8("api_key"), LASTFM_API_KEY)
        .header(Q_UTF8("User-Agent"), Q_UTF8("xamp-player/1.0.0"))
        .success(handler)
        .get();
}

void LastfmClient::downloadImage(int32_t artist_id, const QString &image_url) {
    http::HttpClient(image_url)
        .download([=](const QByteArray &data) {
            QPixmap image;
            image.loadFromData(data);
            if (image.isNull()) {
                return;
            }
            emit onDownloadImage(artist_id, image);
        });
}
