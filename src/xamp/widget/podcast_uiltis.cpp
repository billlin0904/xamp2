#include <QJsonArray>
#include <QJsonDocument>
#include <rapidxml.hpp>

#include <widget/str_utilts.h>
#include <base/str_utilts.h>

#include <widget/podcast_uiltis.h>

using namespace rapidxml;

template <typename Ch>
std::wstring parseCDATA(rapidxml::xml_node<Ch>* node) {
    auto nest_node = node->first_node();
    std::string cddata(nest_node->value(), nest_node->value_size());
    return String::ToString(cddata);
}

Vector<Metadata> parseJson(QString const& json) {
    QJsonParseError error;
    Vector<Metadata> metadatas;

    const auto doc = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (error.error == QJsonParseError::NoError) {
        auto result = doc.array();
        metadatas.reserve(result.size());
        for (const auto entry : result) {
            auto object = entry.toVariant().toMap();
            auto url = object.value(Q_TEXT("url")).toString();
            auto title = object.value(Q_TEXT("title")).toString();
            auto performer = object.value(Q_TEXT("performer")).toString();
            Metadata metadata;
            metadata.file_path = url.toStdWString();
            metadata.title = title.toStdWString();
            metadata.artist = performer.toStdWString();
            metadatas.push_back(metadata);
        }
    }
    return metadatas;
}

std::pair<std::string, ForwardList<Metadata>> parsePodcastXML(QString const& src) {
    auto str = src.toStdString();

    ForwardList<Metadata> metadatas;
    xml_document<> doc;
    doc.parse<0>(str.data());

    auto* rss = doc.first_node("rss");
    if (!rss) {
        return std::make_pair("", metadatas);
    }

    auto* channel = rss->first_node("channel");
    if (!channel) {
        return std::make_pair("", metadatas);
    }

    std::string image_url;
    auto* image = channel->first_node("itunes:image");
    if (image != nullptr) {
        auto href = image->first_attribute("href");
        if (href != nullptr) {
            std::string url_value(href->value(), href->value_size());
            image_url = url_value;
        }        
    }
    size_t i = 1;
    for (auto* item = channel->first_node("item"); item; item = item->next_sibling("item")) {
        Metadata metadata;
        for (auto* node = item->first_node(); node; node = node->next_sibling()) {
            std::string name(node->name(), node->name_size());
            std::string value(node->value(), node->value_size());
            if (name == "title") {
                metadata.title = parseCDATA(node);
                metadata.track = i++;
            }
            else if (name == "dc:creator") {
                metadata.artist = parseCDATA(node);
                metadata.album = parseCDATA(node);
            }
            else if (name == "enclosure") {
                auto* url = node->first_attribute("url");
                if (!url) {
                    continue;
                }
                std::string path(url->value(), url->value_size());
                metadata.file_path = String::ToString(path);
            }
            else if (name == "pubDate") {
                auto datetime = QDateTime::fromString(QString::fromStdString(value), Qt::RFC2822Date);
                metadata.last_write_time = datetime.toTime_t();
            }
        }

        metadatas.push_front(metadata);
    }

    return std::make_pair(image_url, metadatas);
}

std::pair<std::string, ForwardList<Metadata>> parseMbDiscIdXML(QString const& src) {
    auto str = src.toStdString();

    ForwardList<Metadata> metadatas;
    xml_document<> doc;
    doc.parse<0>(str.data());

    auto* metadata = doc.first_node("metadata");
    if (!metadata) {
        return std::make_pair("", metadatas);
    }
    auto* disc = metadata->first_node("disc");
    if (!disc) {
        return std::make_pair("", metadatas);
    }
    auto* release_list = disc->first_node("release-list");
    if (!release_list) {
        return std::make_pair("", metadatas);
    }
    auto* release = release_list->first_node("release");
    if (!release) {
        return std::make_pair("", metadatas);
    }

    auto realease_id_attr  = release->first_attribute("id");
    std::string realease_id(realease_id_attr->value(), realease_id_attr->value_size());
    std::string image_url = "http://coverartarchive.org/release/" + realease_id;
    
    std::wstring album;
    std::wstring artist;
    for (auto* node = release->first_node(); node; node = node->next_sibling()) {
        std::string release_name(node->name(), node->name_size());
        std::string release_value(node->value(), node->value_size());
        if (release_name == "title") {
            album = String::ToStdWString(release_value);
        }
        else if (release_name == "artist-credit") {
            auto* name_credit = node->first_node("name-credit");
            if (!name_credit) {
                return std::make_pair("", metadatas);
            }
            auto* artist_node = name_credit->first_node("artist");
            if (!artist_node) {
                return std::make_pair("", metadatas);
            }
            for (auto* node = artist_node->first_node(); node; node = node->next_sibling()) {
                std::string artist_name(node->name(), node->name_size());
                std::string artist_value(node->value(), node->value_size());
                if (artist_name == "name") {
                    artist = String::ToStdWString(artist_value);
                    break;
                }
            }
        }
        else if (release_name == "medium-list") {
            auto* medium = node->first_node("medium");
            if (!medium) {
                return std::make_pair("", metadatas);
            }
            auto* track_list = medium->first_node("track-list");
            if (!track_list) {
                return std::make_pair("", metadatas);
            }

            Metadata metadata;
            metadata.album = album;
            metadata.artist = artist;

            for (auto* item = track_list->first_node("track"); item; item = item->next_sibling("track")) {
                for (auto* track = item->first_node(); track; track = track->next_sibling()) {
                    std::string track_name(track->name(), track->name_size());
                    std::string track_value(track->value(), track->value_size());
                    if (track_name == "number") {
                        metadata.track = strtoul(track_value.c_str(), nullptr, 10);
                    }
                }

                auto* recording = item->first_node("recording");
                if (!recording) {
                    return std::make_pair("", metadatas);
                }
                
                for (auto* node = recording->first_node(); node; node = node->next_sibling()) {
                    std::string recording_name(node->name(), node->name_size());
                    std::string recording_value(node->value(), node->value_size());
                    if (recording_name == "title") {
                        metadata.title = String::ToStdWString(recording_value);
                        metadatas.push_front(metadata);
                    }
                }
            }
        }
    }    
    return std::make_pair(image_url, metadatas);
}

QString parseCoverUrl(QString const& json) {
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (error.error == QJsonParseError::NoError) {
        auto result = doc[Q_TEXT("images")].toArray();
        for (const auto entry : result) {
            auto object = entry.toVariant().toMap();
            auto front = object.value(Q_TEXT("front")).toBool();
            if (front) {
                return object.value(Q_TEXT("image")).toString();
            }
        }
    }
    return Qt::EmptyString;
}