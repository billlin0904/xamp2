#include <QJsonArray>
#include <QJsonDocument>
#include <rapidxml.hpp>

#include <widget/str_utilts.h>
#include <base/str_utilts.h>

#include <widget/podcast_uiltis.h>

template <typename Ch>
std::wstring parseCDATA(rapidxml::xml_node<Ch>* node) {
    auto nest_node = node->first_node();
    std::string cddata(nest_node->value(), nest_node->value_size());
    return String::ToString(cddata);
}

std::vector<Metadata> parseJson(QString const& json) {
    QJsonParseError error;
    std::vector<Metadata> metadatas;

    const auto doc = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (error.error == QJsonParseError::NoError) {
        auto result = doc.array();
        metadatas.reserve(result.size());
        for (const auto entry : result) {
            auto object = entry.toVariant().toMap();
            auto url = object.value(Q_UTF8("url")).toString();
            auto title = object.value(Q_UTF8("title")).toString();
            auto performer = object.value(Q_UTF8("performer")).toString();
            Metadata metadata;
            metadata.file_path = url.toStdWString();
            metadata.title = title.toStdWString();
            metadata.artist = performer.toStdWString();
            metadatas.push_back(metadata);
        }
    }
    return metadatas;
}

std::pair<std::string, std::vector<Metadata>> parsePodcastXML(QString const& src) {
    auto str = src.toStdString();

    std::vector<Metadata> metadatas;
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
    auto* image = channel->first_node("image");
    if (image != nullptr) {
        auto* url = image->first_node("url");
        if (url != nullptr) {
            std::string url_value(url->value(), url->value_size());
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
                metadata.timestamp = datetime.toTime_t();
            }
        }

        metadatas.push_back(metadata);
    }

    return std::make_pair(image_url, metadatas);
}

