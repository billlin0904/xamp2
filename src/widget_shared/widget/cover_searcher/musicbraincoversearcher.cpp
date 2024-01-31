#include <QUrl>
#include <QString>
#include <QXmlStreamReader>
#include <QImageReader>

#include <base/logger.h>
#include <base/logger_impl.h>

#include <widget/imagecache.h>
#include <widget/http.h>
#include <widget/util/str_utilts.h>
#include <widget/cover_searcher/musicbraincoversearcher.h>

namespace {

constexpr ConstLatin1String kReleaseSearchUrl = "https://musicbrainz.org/ws/2/release/";
constexpr ConstLatin1String kAlbumCoverUrl = "https://coverartarchive.org/release/%1/front";

}

MusicbrainzCoverSearcher::MusicbrainzCoverSearcher(QObject* parent)
	: ICoverSearcher(parent) {
}

void MusicbrainzCoverSearcher::Search(const QString& artist, const QString& album, int id) {
	const QUrl search_url(kReleaseSearchUrl);

	const auto query = qSTR(R"(release:"%1" AND artist:"%2")")
		.arg(album.trimmed().replace('"', "\\\""))
		.arg(artist.trimmed().replace('"', "\\\""));

	http::HttpClient(search_url)
		.param("query", query)
		.param("limit", "5")
		.success([id, album, artist, this](const auto& url, const auto &content) {
		QList<QString> releases;

		QXmlStreamReader reader(content);
		while (!reader.atEnd()) {
			const auto type = reader.readNext();
			if (type == QXmlStreamReader::StartElement && reader.name() == qTEXT("release")) {
				auto release_id = reader.attributes().value("id");
				if (!release_id.isEmpty()) {
					releases.append(release_id.toString());
				}
			}
		}

		auto total_size = releases.size();

		Q_FOREACH(const QString& release_id, releases) {
			QUrl release_url(QString(kAlbumCoverUrl).arg(release_id));
			http::HttpClient(release_url)
				.success([id, total_size, release_id, this](const auto& url, const auto&) {
					http::HttpClient(url)
					.download([id, total_size, url, this](const auto& content) {
						QBuffer buffer;
						buffer.setData(content);
						buffer.open(QIODevice::ReadOnly);
						QImageReader reader(&buffer, "JPG");
						const auto image = reader.read();
						FetchFinished(id, total_size, image);
					});
				})
				.error([id, total_size, release_id, this](const auto& url, const auto& error) {
					FetchFinished(id, total_size, QImage());
				})
				.head();
		}
		}).get();
}

void MusicbrainzCoverSearcher::FetchFinished(int id, qsizetype total_size, const QImage &image) {
	image_list_.insert(id, image);
	if (image_list_.values(id).size() == total_size) {
		auto images = image_list_.values(id);
		const auto null_image_count = 
			std::ranges::count_if(images, [](const auto& a) {
			return a.isNull();
			});
		if (null_image_count != total_size) {
			std::ranges::sort(images, [](const auto& a, const auto& b) {
				return a.bytesPerLine() * a.height()
									> b.bytesPerLine() * b.height();
				});
			emit SearchFinished(id, QPixmap::fromImage(images.first()));
		}
		image_list_.remove(id);
	}
}