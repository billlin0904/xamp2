#include <QImageReader>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

#include <array>
#include <limits>
#include <optional>

#include <base/scopeguard.h>
#include <widget/worker/albumcoverservice.h>
#include <widget/dao/albumdao.h>
#include <widget/dao/musicdao.h>

XAMP_DECLARE_LOG_NAME(AlbumCoverService);

namespace {
	const QStringList kCoverExtensions{
		"*.jpeg"_str,
		"*.jpg"_str,
		"*.png"_str,
	};

	std::optional<QImage> readEmbeddedCoverImage(xamp::metadata::IMetadataReader& reader) {
		const auto buffer = reader.ReadEmbeddedCover();
		if (!buffer) {
			return std::nullopt;
		}

		const auto& data = buffer.value();
		if (data.size() > static_cast<size_t>((std::numeric_limits<int>::max)())) {
			return std::nullopt;
		}

		QImage image;
		if (!image.loadFromData(reinterpret_cast<const uchar*>(data.data()), static_cast<int>(data.size()))) {
			return std::nullopt;
		}
		return image;
	}

	std::optional<QImage> readCoverFileImage(const QString& file_path) {
		QImageReader reader(file_path);
		reader.setAutoTransform(true);

		QImage image;
		if (!reader.read(&image) || image.isNull()) {
			return std::nullopt;
		}
		return image;
	}

	std::optional<QImage> scanCoverImageFromDir(const QString& file_path) {
		const std::array<QString, 3> kTargetFolders = { "scans"_str, "artwork"_str, "booklet"_str };
		constexpr auto kMaxDirCdUp = 4;
		constexpr auto kMaxUnexceptedDirSize = 10;
		const QString kFrontCoverName = "Front"_str;

		const QFileInfo input_info(file_path);
		const QDir dir = input_info.isDir()
			? QDir(input_info.absoluteFilePath())
			: input_info.absoluteDir();
		QDir scan_dir(dir);

		auto find_dir_image = [&](const QDir& target_dir, QDirIterator::IteratorFlags dir_iter_flag) -> std::optional<QImage> {
			QStringList image_file_list;
			for (QDirIterator itr(target_dir.path(), kCoverExtensions, QDir::Files | QDir::NoDotAndDotDot, dir_iter_flag);
				itr.hasNext();) {
				image_file_list.append(itr.next());
			}

			if (image_file_list.isEmpty()) {
				return std::nullopt;
			}

			std::sort(image_file_list.begin(), image_file_list.end(), [](const auto& a, const auto& b) {
				bool ok_a = false;
				bool ok_b = false;
				const auto index_a = QFileInfo(a).baseName().toInt(&ok_a);
				const auto index_b = QFileInfo(b).baseName().toInt(&ok_b);
				if (ok_a && ok_b) {
					return index_a < index_b;
				}
				if (ok_a != ok_b) {
					return ok_a;
				}
				return QString::localeAwareCompare(a, b) < 0;
				});

			auto find_cover_path = image_file_list[0];
			for (const auto& image_file_path : image_file_list) {
				if (image_file_path.contains(kFrontCoverName, Qt::CaseInsensitive)) {
					find_cover_path = image_file_path;
					break;
				}
			}
			return readCoverFileImage(find_cover_path);
			};

		if (auto image = find_dir_image(QDir(dir.absolutePath()), QDirIterator::NoIteratorFlags)) {
			return image;
		}

		auto cd_up_count = 0;
		while (!scan_dir.isRoot() && cd_up_count < kMaxDirCdUp) {
			bool found = false;
			const auto dirs = scan_dir.entryList(QDir::Dirs);
			if (dirs.count() > kMaxUnexceptedDirSize) {
				return std::nullopt;
			}
			for (const auto& folder : kTargetFolders) {
				for (const auto& child_dir : dirs) {
					if (child_dir.contains(folder, Qt::CaseInsensitive)) {
						scan_dir.cd(child_dir);
						found = true;
						break;
					}
				}
				if (found) {
					break;
				}
			}

			if (auto image = find_dir_image(scan_dir, QDirIterator::Subdirectories)) {
				return image;
			}
			scan_dir.cdUp();
			++cd_up_count;
		}

		return std::nullopt;
	}
}

AlbumCoverService::AlbumCoverService()
    : database_ptr_(getPooledDatabase(2)) {
	logger_ = XAMP_LOG_CREATE_LOGGER(AlbumCoverService);
}

void AlbumCoverService::cleanup() {
    database_ptr_.reset();
}

void AlbumCoverService::enableFetchThumbnail(bool enable) {
    enable_ = enable;
}

void AlbumCoverService::cancelRequested() {
    is_stop_ = true;
    pending_album_cover_ids_.clear();
}

void AlbumCoverService::onFindAlbumCover(const DatabaseCoverId& id) {
    is_stop_ = false;

    if (!enable_ || !id.second.has_value()) {
        return;
    }

    const auto album_id = id.second.value();
    if (pending_album_cover_ids_.contains(album_id)) {
        return;
    }
    pending_album_cover_ids_.insert(album_id);
    XAMP_ON_SCOPE_EXIT(
        pending_album_cover_ids_.erase(album_id);
    );

    auto db = database_ptr_->Acquire();
    dao::AlbumDao album_dao(db->getDatabase());
    dao::MusicDao music_dao(db->getDatabase());

    try {
	    const auto cover_id = album_dao.getAlbumCoverId(album_id);
        if (!isNullOfEmpty(cover_id) && cover_id != "unknown_album"_str) {
            return;
        }

        // 1. Read embedded cover in music file.
        auto music_file_path = music_dao.getMusicFilePath(id.first).toStdWString();
        
        if (music_file_path.empty()) {
            // 2. Read embedded cover in album first music file.
            if (auto file_path = album_dao.getAlbumFirstMusicFilePath(album_id)) {
                music_file_path = file_path->toStdWString();
            }            
        }

        // 3. Read file embedded cover.
        if (music_file_path.empty()) {
            return;
        }

        if (!IsFilePath(music_file_path)) {
            return;
        }

        auto reader = MakeMetadataReader();
		reader->Open(music_file_path);
        auto cover = readEmbeddedCoverImage(*reader);
        if (cover && !cover->isNull()) {
            emit albumCoverLoaded(album_id, cover.value(), false);
            return;
        }

		XAMP_LOG_D(logger_, "No embedded cover found in file: {}", QString::fromStdWString(music_file_path).toStdString());

        // 4. If not found embedded cover, try to find cover from album folder.
        cover = scanCoverImageFromDir(QString::fromStdWString(music_file_path));
        if (cover && !cover->isNull()) {
            emit albumCoverLoaded(album_id, cover.value(), true);
            return;
        }

        XAMP_LOG_D(logger_, "No folder cover found in file: {}", QString::fromStdWString(music_file_path).toStdString());
	}
	catch (const std::exception &e) {
        XAMP_LOG_D(logger_, "Find album cover error: {}", e.what());
	}    
}
