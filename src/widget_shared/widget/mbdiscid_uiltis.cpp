#include <widget/mbdiscid_uiltis.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <rapidxml.hpp>

#include <base/str_utilts.h>
#include <stream/filestream.h>
#include <widget/widget_shared.h>
#include <widget/str_utilts.h>

using namespace rapidxml;

template <typename Ch>
std::wstring parseCDATA(rapidxml::xml_node<Ch>* node) {
    auto nest_node = node->first_node();
    std::string cddata(nest_node->value(), nest_node->value_size());
    return String::ToString(cddata);
}

std::pair<std::string, MbDiscIdInfo> ParseMbDiscIdXml(QString const& src) {
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

    auto release_id_attr  = release->first_attribute("id");
    std::string release_id(release_id_attr->value(), release_id_attr->value_size());
    std::string image_url = "http://coverartarchive.org/release/" + release_id;
    
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

QString ParseCoverUrl(QString const& json) {
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        return kEmptyString;
    }
    auto result = doc[qTEXT("images")].toArray();
    for (const auto entry : result) {
        auto object = entry.toVariant().toMap();
        const auto front = object.value(qTEXT("front")).toBool();
        if (front) {
            return object.value(qTEXT("image")).toString();
        }
    }
    return kEmptyString;
}
