#include <QImageReader>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

#include <array>
#include <limits>
#include <optional>

#include <base/scopeguard.h>
#include <base/stopwatch.h>
#include <widget/worker/albumcoverservice.h>
#include <widget/dao/albumdao.h>
#include <widget/dao/musicdao.h>
#include <widget/imagecache.h>

XAMP_DECLARE_LOG_NAME(AlbumCoverService);

namespace {
	const QStringList kCoverExtensions{
		"*.jpeg"_str,
		"*.jpg"_str,
		"*.png"_str,
	};

	std::optional<QImage> readEmbeddedCoverImage(xamp::metadata::IMetadataReader& reader) {
		const auto buffer = reader.readEmbeddedCover();
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
	logger_->setLevel(LogLevel::LOG_LEVEL_DEBUG);
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
    Stopwatch total_elapsed;
    Stopwatch stage_elapsed;

    if (!enable_) {
        XAMP_LOG_D(logger_,
            "Skip album cover request because thumbnail fetch is disabled. music:{} elapsed:{:.3f}s",
            id.first,
            total_elapsed.ElapsedSeconds());
        return;
    }

    if (!id.second.has_value()) {
        XAMP_LOG_D(logger_,
            "Skip album cover request because album id is missing. music:{} elapsed:{:.3f}s",
            id.first,
            total_elapsed.ElapsedSeconds());
        return;
    }

    const auto album_id = id.second.value();
    if (completed_album_cover_ids_.contains(album_id)) {
        XAMP_LOG_D(logger_,
            "Skip completed album cover request. music:{} album:{} elapsed:{:.3f}s",
            id.first,
            album_id,
            total_elapsed.ElapsedSeconds());
        return;
    }

    if (pending_album_cover_ids_.contains(album_id)) {
        XAMP_LOG_D(logger_,
            "Skip duplicated album cover request. music:{} album:{} elapsed:{:.3f}s",
            id.first,
            album_id,
            total_elapsed.ElapsedSeconds());
        return;
    }
    pending_album_cover_ids_.insert(album_id);
    XAMP_ON_SCOPE_EXIT(
        pending_album_cover_ids_.erase(album_id);
    );

    XAMP_LOG_D(logger_, "start finding album cover. music:{} album:{}",
        id.first,
        album_id);

    stage_elapsed.reset();
    auto db = database_ptr_->Acquire();
    const auto acquire_db_elapsed = stage_elapsed.ElapsedSeconds();

    stage_elapsed.reset();
    dao::AlbumDao album_dao(db->getDatabase());
    dao::MusicDao music_dao(db->getDatabase());
    const auto dao_create_elapsed = stage_elapsed.ElapsedSeconds();

    XAMP_LOG_D(logger_,
        "Album cover request database ready. music:{} album:{} acquire_db:{:.3f}s create_dao:{:.3f}s total:{:.3f}s",
        id.first,
        album_id,
        acquire_db_elapsed,
        dao_create_elapsed,
        total_elapsed.ElapsedSeconds());

    try {
        stage_elapsed.reset();
	    const auto cover_id = album_dao.getAlbumCoverId(album_id);
        const auto get_cover_id_elapsed = stage_elapsed.ElapsedSeconds();

        XAMP_LOG_D(logger_,
            "Album cover id lookup completed. music:{} album:{} cover:{} elapsed:{:.3f}s total:{:.3f}s",
            id.first,
            album_id,
            cover_id.toStdString(),
            get_cover_id_elapsed,
            total_elapsed.ElapsedSeconds());

        if (!isNullOfEmpty(cover_id) && cover_id != "unknown_album"_str) {
            stage_elapsed.reset();
            if (qImageCache.isFileExists(QString{}, cover_id)) {
                const auto cache_exists_elapsed = stage_elapsed.ElapsedSeconds();
                completed_album_cover_ids_.insert(album_id);
                XAMP_LOG_D(logger_,
                    "Album cover already exists in database. music:{} album:{} cover:{} cache_exists:{:.3f}s total:{:.3f}s",
                    id.first,
                    album_id,
                    cover_id.toStdString(),
                    cache_exists_elapsed,
                    total_elapsed.ElapsedSeconds());
                return;
            }
            const auto cache_exists_elapsed = stage_elapsed.ElapsedSeconds();

            XAMP_LOG_D(logger_,
                "Album cover id exists but cache file is missing. music:{} album:{} cover:{} cache_exists:{:.3f}s total:{:.3f}s",
                id.first,
                album_id,
                cover_id.toStdString(),
                cache_exists_elapsed,
                total_elapsed.ElapsedSeconds());
        }

        // 1. read embedded cover in music file.
        stage_elapsed.reset();
        auto music_file_path = music_dao.getMusicFilePath(id.first).toStdWString();
        const auto music_path_lookup_elapsed = stage_elapsed.ElapsedSeconds();

        XAMP_LOG_D(logger_,
            "Album cover music path lookup completed. music:{} album:{} has_path:{} elapsed:{:.3f}s total:{:.3f}s",
            id.first,
            album_id,
            !music_file_path.empty(),
            music_path_lookup_elapsed,
            total_elapsed.ElapsedSeconds());

        if (music_file_path.empty()) {
            // 2. read embedded cover in album first music file.
            stage_elapsed.reset();
            if (auto file_path = album_dao.getAlbumFirstMusicFilePath(album_id)) {
                music_file_path = file_path->toStdWString();
            }
            const auto album_first_path_elapsed = stage_elapsed.ElapsedSeconds();

            XAMP_LOG_D(logger_,
                "Album cover first album music path lookup completed. music:{} album:{} has_path:{} elapsed:{:.3f}s total:{:.3f}s",
                id.first,
                album_id,
                !music_file_path.empty(),
                album_first_path_elapsed,
                total_elapsed.ElapsedSeconds());
        }

        // 3. read file embedded cover.
        if (music_file_path.empty()) {
            XAMP_LOG_D(logger_,
                "Skip album cover request because music file path is empty. music:{} album:{} total:{:.3f}s",
                id.first,
                album_id,
                total_elapsed.ElapsedSeconds());
            return;
        }

        const auto music_file_path_string = QString::fromStdWString(music_file_path);
        if (!IsFilePath(music_file_path)) {
            XAMP_LOG_D(logger_,
                "Skip album cover request because path is not a file. music:{} album:{} file:{} total:{:.3f}s",
                id.first,
                album_id,
                music_file_path_string.toStdString(),
                total_elapsed.ElapsedSeconds());
            return;
        }

        stage_elapsed.reset();
        auto reader = makeMetadataReader();
        const auto make_reader_elapsed = stage_elapsed.ElapsedSeconds();

        stage_elapsed.reset();
		reader->open(music_file_path);
        const auto open_reader_elapsed = stage_elapsed.ElapsedSeconds();

        stage_elapsed.reset();
        auto cover = readEmbeddedCoverImage(*reader);
        const auto read_embedded_elapsed = stage_elapsed.ElapsedSeconds();

        XAMP_LOG_D(logger_,
            "Album cover embedded read completed. music:{} album:{} file:{} found:{} make_reader:{:.3f}s open:{:.3f}s read:{:.3f}s total:{:.3f}s",
            id.first,
            album_id,
            music_file_path_string.toStdString(),
            cover.has_value() && !cover->isNull(),
            make_reader_elapsed,
            open_reader_elapsed,
            read_embedded_elapsed,
            total_elapsed.ElapsedSeconds());

        if (cover && !cover->isNull()) {
            completed_album_cover_ids_.insert(album_id);
            XAMP_LOG_D(logger_,
                "Embedded album cover found. music:{} album:{} file:{} size:{}x{} total:{:.3f}s",
                id.first,
                album_id,
                music_file_path_string.toStdString(),
                cover->width(),
                cover->height(),
                total_elapsed.ElapsedSeconds());
            stage_elapsed.reset();
            emit albumCoverLoaded(album_id, cover.value(), false);
            XAMP_LOG_D(logger_,
                "Embedded album cover emitted. music:{} album:{} emit:{:.3f}s total:{:.3f}s",
                id.first,
                album_id,
                stage_elapsed.ElapsedSeconds(),
                total_elapsed.ElapsedSeconds());
            return;
        }

		XAMP_LOG_D(logger_,
            "No embedded cover found in file. music:{} album:{} file:{} total:{:.3f}s",
            id.first,
            album_id,
            music_file_path_string.toStdString(),
            total_elapsed.ElapsedSeconds());

        // 4. If not found embedded cover, try to find cover from album folder.
        stage_elapsed.reset();
        cover = scanCoverImageFromDir(music_file_path_string);
        const auto scan_folder_elapsed = stage_elapsed.ElapsedSeconds();

        XAMP_LOG_D(logger_,
            "Album cover folder scan completed. music:{} album:{} file:{} found:{} elapsed:{:.3f}s total:{:.3f}s",
            id.first,
            album_id,
            music_file_path_string.toStdString(),
            cover.has_value() && !cover->isNull(),
            scan_folder_elapsed,
            total_elapsed.ElapsedSeconds());

        if (cover && !cover->isNull()) {
            completed_album_cover_ids_.insert(album_id);
            XAMP_LOG_D(logger_,
                "Folder album cover found. music:{} album:{} file:{} size:{}x{} total:{:.3f}s",
                id.first,
                album_id,
                music_file_path_string.toStdString(),
                cover->width(),
                cover->height(),
                total_elapsed.ElapsedSeconds());
            stage_elapsed.reset();
            emit albumCoverLoaded(album_id, cover.value(), true);
            XAMP_LOG_D(logger_,
                "Folder album cover emitted. music:{} album:{} emit:{:.3f}s total:{:.3f}s",
                id.first,
                album_id,
                stage_elapsed.ElapsedSeconds(),
                total_elapsed.ElapsedSeconds());
            return;
        }

        XAMP_LOG_D(logger_,
            "No folder cover found in file. music:{} album:{} file:{} total:{:.3f}s",
            id.first,
            album_id,
            music_file_path_string.toStdString(),
            total_elapsed.ElapsedSeconds());
	}
	catch (const std::exception &e) {
        XAMP_LOG_D(logger_,
            "Find album cover error. music:{} album:{} error:{} total:{:.3f}s",
            id.first,
            album_id,
            e.what(),
            total_elapsed.ElapsedSeconds());
	}    
}
