#include <QJsonArray>
#include <QJsonDocument>
#include <rapidxml.hpp>

#include <base/str_utilts.h>
#include <stream/filestream.h>

#include <widget/str_utilts.h>
#include <widget/podcast_uiltis.h>

using namespace rapidxml;

template <typename Ch>
std::wstring parseCDATA(rapidxml::xml_node<Ch>* node) {
    auto nest_node = node->first_node();
    std::string cddata(nest_node->value(), nest_node->value_size());
    return String::ToString(cddata);
}

ForwardList<TrackInfo> parseJson(QString const& json) {
    QJsonParseError error;
    ForwardList<TrackInfo> track_infos;

    auto track = 1;
    const auto doc = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (error.error == QJsonParseError::NoError) {
        auto result = doc.array();
        for (const auto entry : result) {
            auto object = entry.toVariant().toMap();
            auto url = object.value(qTEXT("url")).toString();
            auto artist = object.value(qTEXT("artist")).toString().toStdWString();
            auto title = object.value(qTEXT("title")).toString();
            auto performer = object.value(qTEXT("performer")).toString();
            auto dateTime = object.value(qTEXT("datetime")).toDateTime();
            TrackInfo track_info;
            track_info.file_path = url.toStdWString();
            if (artist == performer.toStdWString()) {
                track_info.title = title.toStdWString() + L" (" + dateTime.toString(qTEXT("yyyy-MM-dd")).toStdWString() + L") ";
            } else {
                track_info.title = title.toStdWString() + L" (" + dateTime.toString(qTEXT("yyyy-MM-dd")).toStdWString() + L") " + L" (Ori. " + artist + L")";
            }
            track_info.artist = performer.toStdWString();
            track_info.album = L"Podcast";
            track_info.track = track++;
            track_infos.push_front(track_info);
        }
    }
    return track_infos;
}

std::pair<std::string, ForwardList<TrackInfo>> parsePodcastXML(QString const& src) {
    auto str = src.toStdString();

    ForwardList<TrackInfo> metadatas;
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
        TrackInfo track_info;
        for (auto* node = item->first_node(); node; node = node->next_sibling()) {
            std::string name(node->name(), node->name_size());
            std::string value(node->value(), node->value_size());
            if (name == "title") {
                track_info.title = parseCDATA(node);
                track_info.track = i++;
            }
            else if (name == "dc:creator") {
                track_info.artist = parseCDATA(node);
                track_info.album = parseCDATA(node);
            }
            else if (name == "enclosure") {
                auto* url = node->first_attribute("url");
                if (!url) {
                    continue;
                }
                std::string path(url->value(), url->value_size());
                track_info.file_path = String::ToString(path);
            }
        }
        metadatas.push_front(track_info);
    }

    return std::make_pair(image_url, metadatas);
}

std::pair<std::string, MbDiscIdInfo> parseMbDiscIdXML(QString const& src) {
    auto str = src.toStdString();

    MbDiscIdInfo mb_disc_id_info;

    xml_document<> doc;
    doc.parse<0>(str.data());

    auto* metadata = doc.first_node("metadata");
    if (!metadata) {
        return std::make_pair("", mb_disc_id_info);
    }
    auto* disc = metadata->first_node("disc");
    if (!disc) {
        return std::make_pair("", mb_disc_id_info);
    }
    auto* release_list = disc->first_node("release-list");
    if (!release_list) {
        return std::make_pair("", mb_disc_id_info);
    }
    auto* release = release_list->first_node("release");
    if (!release) {
        return std::make_pair("", mb_disc_id_info);
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
                return std::make_pair("", mb_disc_id_info);
            }
            auto* artist_node = name_credit->first_node("artist");
            if (!artist_node) {
                return std::make_pair("", mb_disc_id_info);
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
                return std::make_pair("", mb_disc_id_info);
            }
            auto* track_list = medium->first_node("track-list");
            if (!track_list) {
                return std::make_pair("", mb_disc_id_info);
            }

            mb_disc_id_info.album = album;
            mb_disc_id_info.artist = artist;

            for (auto* item = track_list->first_node("track"); item; item = item->next_sibling("track")) {
                MbDiscIdTrack mb_disc_id_track;

                for (auto* track = item->first_node(); track; track = track->next_sibling()) {
                    std::string track_name(track->name(), track->name_size());
                    std::string track_value(track->value(), track->value_size());
                    if (track_name == "number") {
                        mb_disc_id_track.track = strtoul(track_value.c_str(), nullptr, 10);
                    }
                }

                auto* recording = item->first_node("recording");
                if (!recording) {
                    return std::make_pair("", mb_disc_id_info);
                }
                
                for (auto* node = recording->first_node(); node; node = node->next_sibling()) {
                    std::string recording_name(node->name(), node->name_size());
                    std::string recording_value(node->value(), node->value_size());
                    if (recording_name == "title") {
                        mb_disc_id_track.title = String::ToStdWString(recording_value);
                        mb_disc_id_info.tracks.push_front(mb_disc_id_track);
                    }
                }
            }
        }
    }    
    return std::make_pair(image_url, mb_disc_id_info);
}

QString parseCoverUrl(QString const& json) {
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        return qEmptyString;
    }
    auto result = doc[qTEXT("images")].toArray();
    for (const auto entry : result) {
        auto object = entry.toVariant().toMap();
        const auto front = object.value(qTEXT("front")).toBool();
        if (front) {
            return object.value(qTEXT("image")).toString();
        }
    }
    return qEmptyString;
}
