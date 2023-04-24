#include <widget/kkbox_utilis.h>
#include <widget/str_utilts.h>
#include <QJsonArray>
#include <QJsonDocument>

namespace kkbox {

static std::optional<kkbox::ArtistData> ParseArtistData(const QJsonDocument& doc, const QString& find_artist) {
    auto artists = doc[qTEXT("artists")][qTEXT("data")].toArray();
    std::optional<kkbox::ArtistData> result = std::nullopt;
    bool found = false;

    for (auto artist : artists) {        
        auto object = artist.toVariant().toMap();
        auto name = object.value(qTEXT("name")).toString();
        if (name != find_artist) {
            continue;
        }

        found = true;
        auto id = object.value(qTEXT("id")).toString();
        auto url = object.value(qTEXT("url")).toString();
        auto images = object.value(qTEXT("images")).toJsonArray();

        kkbox::ArtistData data;
        data.id = id;
        data.name = name;
        data.url = url;

        for (auto image : images) {
            auto image_object = image.toVariant().toMap();
            auto width = image_object.value(qTEXT("width")).toInt();
			auto height = image_object.value(qTEXT("height")).toInt();
			auto url = image_object.value(qTEXT("url")).toString();
            kkbox::ImageData image;
			image.width = width;
			image.height = height;
			image.url = url;
            data.images.push_back(image);
        }
        if (found) {
            result = data;
            break;
        }
    }
    return result;
}

std::optional<ArtistData> ParseArtistData(const QString& json, const QString& find_artist) {
    auto doc = QJsonDocument::fromJson(json.toUtf8());
	return ParseArtistData(doc, find_artist);
}

}
