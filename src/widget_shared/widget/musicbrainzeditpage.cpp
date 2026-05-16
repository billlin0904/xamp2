#include <QStandardItemModel>
#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QHash>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <ui_musicbrainzeditpage.h>
#include <widget/dao/dbfacade.h>
#include <widget/musicbrainzeditpage.h>
#include <widget/tagio.h>
#include <widget/util/image_util.h>
#include <widget/util/ui_util.h>
#include <widget/xmessagebox.h>

#include <algorithm>
#include <cmath>

namespace {
    constexpr double kMinVisibleAlbumScore = 0.55;

    struct CandidateAlbum {
        MusicBrainzRecording recording;
        QList<musicbrain::TrackMatchResult> matches;
        double trackScore = 0;
        double albumScore = 0;
    };

    struct PendingReleaseLookup {
        QList<PlayListEntity> entities;
        QList<musicbrain::Release> candidateReleases;
    };

    musicbrain::FileMeta makeFileMeta(const PlayListEntity& entity, int total_album_tracks) {
        musicbrain::FileMeta meta;
        meta.title = entity.title;
        meta.album = entity.album;
        meta.artist = entity.artist;
        meta.albumArtist = entity.artist;
        meta.trackNumber = static_cast<int>(entity.track);
        meta.totalTracks = total_album_tracks;
        meta.totalAlbumTracks = total_album_tracks;
        meta.lengthMs = static_cast<int>(std::round(entity.duration * 1000.0));
        if (entity.year > 0) {
            meta.date = QString::number(entity.year);
        }
        return meta;
    }

    QString similarityText(double similarity) {
        return QStringLiteral("%1%").arg(QString::number(std::round(similarity * 100.0), 'f', 0));
    }

    QString luceneQuoted(const QString& value) {
        auto escaped = value;
        escaped.replace(QLatin1Char('\\'), "\\\\"_str);
        escaped.replace(QLatin1Char('"'), "\\\""_str);
        return qFormat("\"%1\""_str).arg(escaped);
    }

    QString buildMusicBrainzReleaseQuery(const QList<PlayListEntity>& entities) {
        QStringList parts;
        if (entities.isEmpty()) {
            return QString();
        }
        const auto& entity = entities.front();
        if (!entity.artist.isEmpty()) {
            parts.append("artist:"_str + luceneQuoted(entity.artist));
        }
        if (!entity.album.isEmpty()) {
            parts.append("release:"_str + luceneQuoted(entity.album));
        }
        if (!entities.isEmpty()) {
            parts.append("tracks:"_str + QString::number(entities.count()));
        }
        if (entity.year > 0) {
            parts.append("date:"_str + QString::number(entity.year));
        }
        return parts.join(" AND "_str);
    }

    QString artistText(const musicbrain::TrackInfo& track) {
        return track.artistCredits.join(","_str);
    }

    QString artistCreditText(const QList<musicbrain::ArtistCredit>& credits) {
        QStringList artists;
        for (const auto& credit : credits) {
            if (!credit.name.isEmpty()) {
                artists.append(credit.name);
            }
            else if (!credit.artist.name.isEmpty()) {
                artists.append(credit.artist.name);
            }
        }
        return artists.join(","_str);
    }

    QString artistCreditIds(const QList<musicbrain::ArtistCredit>& credits) {
        QStringList ids;
        for (const auto& credit : credits) {
            if (!credit.artist.id.isEmpty()) {
                ids.append(credit.artist.id);
            }
        }
        return ids.join(";"_str);
    }

    QString releaseTypeText(const musicbrain::ReleaseGroup& release_group) {
        QStringList types;
        if (!release_group.primaryType.isEmpty()) {
            types.append(release_group.primaryType);
        }
        types.append(release_group.secondaryTypes);
        return types.join(";"_str);
    }

    QString releaseDateText(const musicbrain::Release& release) {
        if (!release.date.isEmpty()) {
            return release.date;
        }
        if (!release.events.isEmpty()) {
            return release.events.front().date;
        }
        return release.releaseGroup.firstReleaseDate;
    }

    QString releaseCountryText(const musicbrain::Release& release) {
        if (!release.country.isEmpty()) {
            return release.country;
        }
        if (!release.events.isEmpty() && !release.events.front().area.iso3166_1.isEmpty()) {
            return release.events.front().area.iso3166_1.join(","_str);
        }
        return QString();
    }

    QString releaseDisplayTitle(const MusicBrainzRecording& recording) {
        auto title = recording.title;
        const auto scorePos = title.lastIndexOf(QRegularExpression(QStringLiteral("\\s+\\d+%$")));
        if (scorePos > 0) {
            title = title.left(scorePos);
        }
        return title;
    }

    std::optional<musicbrain::Release> releaseForRecording(const MusicBrainzRecording& recording) {
        for (const auto& release : recording.root_recording.releases) {
            if (release.id == recording.release_id) {
                return release;
            }
        }

        for (const auto& track : recording.tracks) {
            for (const auto& release : track.releases) {
                if (release.id == recording.release_id) {
                    return release;
                }
            }
        }

        return std::nullopt;
    }

    QString albumDisplayTitle(const MusicBrainzRecording& recording) {
        const auto release = releaseForRecording(recording);
        if (release.has_value() && !release->title.isEmpty()) {
            return release->title;
        }
        return releaseDisplayTitle(recording);
    }

    QString recordingDisplayTitle(const MusicBrainzRecording& recording) {
        auto title = recording.root_recording.title;
        if (title.isEmpty()) {
            return releaseDisplayTitle(recording);
        }
        if (!recording.root_recording.disambiguation.isEmpty()) {
            title = qFormat("%1 (%2)").arg(title).arg(recording.root_recording.disambiguation);
        }
        if (recording.root_recording.video) {
            title = qFormat("%1 [%2]").arg(title).arg(QObject::tr("Video"));
        }
        return title;
    }

    QList<musicbrain::TrackMatchResult> matchFilesToTracks(const QList<musicbrain::FileMeta>& metas,
        const QList<musicbrain::TrackInfo>& tracks) {
        QList<musicbrain::TrackMatchResult> matches;
        QVector<bool> usedTracks(tracks.size(), false);

        for (int fileIndex = 0; fileIndex < metas.size(); ++fileIndex) {
            musicbrain::TrackMatchResult best;
            best.fileIndex = fileIndex;
            for (int trackIndex = 0; trackIndex < tracks.size(); ++trackIndex) {
                if (usedTracks[trackIndex]) {
                    continue;
                }
                const auto similarity = musicbrain::compareToTrack(metas[fileIndex], tracks[trackIndex]);
                if (similarity > best.similarity) {
                    best.similarity = similarity;
                    best.trackIndex = trackIndex;
                }
            }
            if (best.trackIndex >= 0) {
                usedTracks[best.trackIndex] = true;
                matches.append(best);
            }
        }
        return matches;
    }

    double averageSimilarity(const QList<musicbrain::TrackMatchResult>& matches) {
        if (matches.isEmpty()) {
            return 0;
        }
        double total = 0;
        for (const auto& match : matches) {
            total += match.similarity;
        }
        return total / matches.size();
    }

    QList<musicbrain::TrackMatchResult> matchRecordingTracks(const QList<musicbrain::FileMeta>& metas,
        const QList<musicbrain::TrackInfo>& tracks) {
        if (tracks.size() != 1) {
            return matchFilesToTracks(metas, tracks);
        }

        const auto& track = tracks.front();
        musicbrain::TrackMatchResult best;
        best.trackIndex = 0;

        for (int fileIndex = 0; fileIndex < metas.size(); ++fileIndex) {
            if (metas[fileIndex].trackNumber == track.trackNo) {
                best.fileIndex = fileIndex;
                best.similarity = musicbrain::compareToTrack(metas[fileIndex], track);
                return { best };
            }
        }

        for (int fileIndex = 0; fileIndex < metas.size(); ++fileIndex) {
            const auto similarity = musicbrain::compareToTrack(metas[fileIndex], track);
            if (similarity > best.similarity) {
                best.fileIndex = fileIndex;
                best.similarity = similarity;
            }
        }

        return best.fileIndex >= 0 ? QList<musicbrain::TrackMatchResult>{ best } : QList<musicbrain::TrackMatchResult>{};
    }

    void appendTrackRowSorted(QStandardItem* album_item,
        const QList<QStandardItem*>& row_items,
        int track_no) {
        auto insertRow = album_item->rowCount();
        for (int row = 0; row < album_item->rowCount(); ++row) {
            const auto itemTrackNo = album_item->child(row, 2)->text().toInt();
            if (track_no > 0 && itemTrackNo > track_no) {
                insertRow = row;
                break;
            }
        }
        album_item->insertRow(insertRow, row_items);
    }

    QString candidateKey(const MusicBrainzRecording& recording) {
        const auto recordingKey = recording.root_recording.id.isEmpty()
            ? qFormat("%1|%2")
                .arg(recording.root_recording.title)
                .arg(recording.tracks.isEmpty() ? 0 : recording.tracks.front().trackNo)
            : recording.root_recording.id;
        return QStringLiteral("%1|%2")
            .arg(recordingKey)
            .arg(recording.release_id);
    }

    QSet<QString> releaseIds(const QList<musicbrain::Release>& candidate_releases) {
        QSet<QString> ids;
        for (const auto& release : candidate_releases) {
            if (!release.id.isEmpty()) {
                ids.insert(release.id);
            }
        }
        return ids;
    }

    int uniqueRecordingCount(const QList<musicbrain::TrackInfo>& tracks) {
        QSet<QString> ids;
        int anonymous = 0;
        for (const auto& track : tracks) {
            if (track.recordingId.isEmpty()) {
                ++anonymous;
            }
            else {
                ids.insert(track.recordingId);
            }
        }
        return ids.size() + anonymous;
    }

    musicbrain::RootRecording rootRecordingFromTrack(const musicbrain::TrackInfo& track,
        const musicbrain::Release& release) {
        musicbrain::RootRecording recording;
        recording.id = track.recordingId;
        recording.title = track.title;
        recording.lengthMs = track.lengthMs;
        recording.video = track.video;
        recording.releases.append(release);
        for (const auto& artist : track.artistCredits) {
            musicbrain::ArtistCredit credit;
            credit.name = artist;
            recording.artistCredits.append(credit);
        }
        return recording;
    }
}

MusicbrainzEditPage::MusicbrainzEditPage(const QList<PlayListEntity>& entities, QWidget* parent)
    : QFrame(parent)
    , nam_(this)
    , http_client_(&nam_, QString(), this) {
    ui_ = new Ui::MusicbrainzEditPage();
    track_model_ = new QStandardItemModel(this);
    album_track_model_ = new QStandardItemModel(this);
	tag_model_ = new QStandardItemModel(this);
    ui_->setupUi(this);

    write_tag_button_ = new QPushButton(tr("Write Tags"), this);
    write_tag_button_->setEnabled(false);

    fetch_progress_bar_ = new QProgressBar(this);
    fetch_progress_bar_->setTextVisible(true);
    fetch_progress_bar_->setRange(0, 1);
    fetch_progress_bar_->setValue(0);
    fetch_progress_bar_->setFormat(tr("Fetching (0/0) album"));
    fetch_progress_bar_->setMinimumWidth(260);

    auto* button_layout = new QHBoxLayout();
    button_layout->addWidget(fetch_progress_bar_, 1);
    button_layout->addStretch();
    button_layout->addWidget(write_tag_button_);
    ui_->verticalLayout_2->addLayout(button_layout);

    (void)QObject::connect(write_tag_button_, &QPushButton::clicked, this, &MusicbrainzEditPage::writeSelectedTag);

	load(entities);
    startFetchMusicBrainzRecording().then([] {
    });
}

MusicbrainzEditPage::~MusicbrainzEditPage() {
    delete ui_;
}

void MusicbrainzEditPage::load(const QList<PlayListEntity>& entities) {
    if (entities.isEmpty()) {
        return;
    }

    entities_ = entities;
    metas_.clear();
    recording_list_.clear();
    track_model_->clear();
    album_track_model_->clear();
    tag_model_->clear();
    cover_art_map_.clear();

    auto f = qTheme.defaultFont();
    f.setPointSize(qTheme.fontSize(9));
    setFont(f);
    ui_->trackView->setFont(f);
    ui_->albumRecordingView->setFont(f);
    ui_->tagTableView->setFont(f);
    ui_->groupBox->setFont(f);
    ui_->coverArtSize->setFont(f);
    if (fetch_progress_bar_ != nullptr) {
        fetch_progress_bar_->setFont(f);
    }
    if (write_tag_button_ != nullptr) {
        write_tag_button_->setFont(f);
    }

    QStringList headers;
    headers << tr("Track") << tr("Title") << tr("Duration");
    track_model_->setColumnCount(headers.size());
    track_model_->setHorizontalHeaderLabels(headers);
    ui_->trackView->setModel(track_model_);
    ui_->trackView->setRootIsDecorated(true);
    ui_->trackView->setAllColumnsShowFocus(true);

    auto* album_item = new QStandardItem(entities.front().album);

    QList<QStandardItem*> top_row;
    top_row << album_item
        << new QStandardItem(QString())
        << new QStandardItem(QString());
    track_model_->appendRow(top_row);

    for (const auto& entity : entities) {
        auto* child1 = new QStandardItem(QString::number(entity.track));
        auto* child2 = new QStandardItem(entity.title);
        auto* child3 = new QStandardItem(formatDuration(entity.duration));
        child3->setData(QVariant::fromValue(entity), Qt::UserRole + 1);
        QList<QStandardItem*> row_items;
        row_items << child1 << child2 << child3;
        album_item->appendRow(row_items);
        metas_.append(makeFileMeta(entity, entities.count()));
    }

    ui_->trackView->expand(album_item->index());
    ui_->trackView->setModel(track_model_);

    QStringList album_track_headers;
    album_track_headers << tr("Album / Tracks") << tr("Match") << tr("Track") << tr("Duration");
    album_track_model_->setColumnCount(album_track_headers.size());
    album_track_model_->setHorizontalHeaderLabels(album_track_headers);
    ui_->albumRecordingView->setModel(album_track_model_);
    ui_->albumRecordingView->setRootIsDecorated(true);
    ui_->albumRecordingView->setAllColumnsShowFocus(true);

    (void)QObject::connect(ui_->albumRecordingView, &QTreeView::clicked,
        [this](const auto& index) {
        if (!index.isValid())
            return;
        auto idx = index.siblingAtColumn(3);
        auto release_id = idx.data(Qt::UserRole + 1).toString();
        auto itr = cover_art_map_.find(release_id);
        if (release_id.isEmpty())
            return;
        if (itr.value().isNull()) {
            ui_->coverArt->setPixmap(QPixmap());
            ui_->coverArtSize->setText(""_str);
            return;
        }
        ui_->coverArt->setPixmap(image_util::resizeImage(itr.value(), QSize(300, 300)));
		ui_->coverArtSize->setText(tr("%1x%2").arg(itr.value().width()).arg(itr.value().height()));
        });

    ui_->trackView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui_->albumRecordingView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui_->trackView->header()->setFont(f);
    ui_->albumRecordingView->header()->setFont(f);

    tag_model_->setColumnCount(3);
    tag_model_->setHeaderData(0, Qt::Horizontal, tr("Tag"));
    tag_model_->setHeaderData(1, Qt::Horizontal, tr("Old value"));
    tag_model_->setHeaderData(2, Qt::Horizontal, tr("New value"));

	ui_->tagTableView->setModel(tag_model_);
    ui_->tagTableView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
    ui_->tagTableView->setFocusPolicy(Qt::StrongFocus);
    auto* header = ui_->tagTableView->horizontalHeader();
    header->setFont(f);
    header->setStretchLastSection(false);
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::Stretch);
    setTabViewStyle(ui_->tagTableView);
    setTagPreview(entities.front(), std::nullopt, QString());

    (void)QObject::connect(ui_->albumRecordingView, &QTreeView::clicked,
        [this](const auto& index) {
        auto idx = index.siblingAtColumn(3);
        if (!index.isValid())
           return;
        auto data = idx.data(Qt::UserRole + 2);
        if (!data.isValid()) {
           selected_track_.reset();
           selected_album_.clear();
           write_tag_button_->setEnabled(false);
           return;
        }
        auto tracks = data.template value<musicbrain::TrackInfo>();
        data = idx.data(Qt::UserRole + 3);
        if (!data.isValid())
            return;
        auto album = data.template value<QString>();
        auto entity = entityForTrack(tracks.trackNo);
        if (entity.has_value()) {
            selectTrackViewTrack(tracks.trackNo);
        }
        setTagPreview(entity, tracks, album);
        selected_entity_ = entity;
        selected_track_ = tracks;
        selected_album_ = album;
        write_tag_button_->setEnabled(entity.has_value() && !entity->is_cue_file && entity->isFilePath());
        });

    (void)QObject::connect(ui_->trackView, &QTreeView::clicked,
        [this](const auto& index) {
        auto idx = index.siblingAtColumn(2);
        if (!index.isValid())
            return;
        auto data = idx.data(Qt::UserRole + 1);
        if (!data.isValid())
            return;        
        auto entity = data.template value<PlayListEntity>();
        selected_entity_ = entity;
        selected_track_.reset();
        selected_album_.clear();
        write_tag_button_->setEnabled(false);
        setTagPreview(entity, std::nullopt, QString());
        });

}

void MusicbrainzEditPage::rebuildCandidateView() {
    cover_art_map_.clear();
    album_track_model_->removeRows(0, album_track_model_->rowCount());

    QList<CandidateAlbum> candidates;
    QSet<QString> seenCandidates;
    for (const auto& albums : recording_list_) {
        for (const auto& recording : albums.recordings) {
            if (recording.tracks.isEmpty()) {
                continue;
            }

            const auto matches = matchRecordingTracks(metas_, recording.tracks);
            const auto trackScore = averageSimilarity(matches);
            const auto albumScore = trackScore > 0
                ? recording.similarity * 0.4 + trackScore * 0.6
                : recording.similarity;

            if (albumScore < kMinVisibleAlbumScore) {
                continue;
            }

            const auto key = candidateKey(recording);
            if (seenCandidates.contains(key)) {
                continue;
            }
            seenCandidates.insert(key);
            candidates.append({ recording, matches, trackScore, albumScore });
        }
    }

    std::sort(candidates.begin(), candidates.end(), [](const auto& left, const auto& right) {
        return left.albumScore > right.albumScore;
    });

    QHash<QString, QStandardItem*> albumItems;
    bool expandedBestAlbum = false;
    for (const auto& candidate : candidates) {
        const auto& recording = candidate.recording;
        QHash<int, double> trackSimilarities;
        for (const auto& match : candidate.matches) {
            trackSimilarities.insert(match.trackIndex, match.similarity);
        }

        const auto albumTitle = albumDisplayTitle(recording);
        const auto releaseTitle = releaseDisplayTitle(recording);
        const auto albumKey = recording.release_id.isEmpty() ? releaseTitle.toCaseFolded() : recording.release_id;

        auto* albumItem = albumItems.value(albumKey, nullptr);
        if (albumItem == nullptr) {
            albumItem = new QStandardItem(releaseTitle);
            QList<QStandardItem*> albumRow;
            albumRow << albumItem
                << new QStandardItem(similarityText(candidate.albumScore))
                << new QStandardItem(QString())
                << new QStandardItem(QString());
            albumRow[3]->setData(recording.release_id, Qt::UserRole + 1);
            albumRow[3]->setData(albumTitle, Qt::UserRole + 3);
            album_track_model_->appendRow(albumRow);
            albumItems.insert(albumKey, albumItem);

            if (!expandedBestAlbum) {
                ui_->albumRecordingView->expand(albumItem->index());
                expandedBestAlbum = true;
            }
        } else {
            auto* scoreItem = album_track_model_->itemFromIndex(albumItem->index().siblingAtColumn(1));
            const auto currentScore = scoreItem != nullptr ? scoreItem->text() : QString();
            if (currentScore.isEmpty() || currentScore.chopped(1).toDouble() < candidate.albumScore * 100.0) {
                if (scoreItem != nullptr) {
                    scoreItem->setText(similarityText(candidate.albumScore));
                }
            }
        }

        cover_art_map_.insert(recording.release_id, recording.cover_art);

        for (int i = 0; i < recording.tracks.size(); ++i) {
            const auto& track = recording.tracks[i];
            auto* trackItem = new QStandardItem(track.title);
            auto* trackScoreItem = new QStandardItem(trackSimilarities.contains(i) ? similarityText(trackSimilarities.value(i)) : QString());
            auto* trackNumberItem = new QStandardItem(QString::number(track.trackNo));
            auto* trackDurationItem = new QStandardItem(track.lengthMs > 0 ? formatDuration(track.lengthMs / 1000.0) : QString());
            trackDurationItem->setData(recording.release_id, Qt::UserRole + 1);
            trackDurationItem->setData(QVariant::fromValue(track), Qt::UserRole + 2);
            trackDurationItem->setData(albumTitle, Qt::UserRole + 3);
            appendTrackRowSorted(albumItem, { trackItem, trackScoreItem, trackNumberItem, trackDurationItem }, track.trackNo);
        }
    }

    ui_->albumRecordingView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

void MusicbrainzEditPage::appendMusicBrainzAlbum(const MusicBrainzAlbum& album) {
    if (album.recordings.isEmpty()) {
        return;
    }
    recording_list_.append(album);
    rebuildCandidateView();
}

std::optional<PlayListEntity> MusicbrainzEditPage::entityForTrack(int track_no) const {
    for (const auto& entity : entities_) {
        if (static_cast<int>(entity.track) == track_no) {
            return entity;
        }
    }
    return entities_.isEmpty() ? std::optional<PlayListEntity>{} : std::optional<PlayListEntity>{ entities_.front() };
}

void MusicbrainzEditPage::selectTrackViewTrack(int track_no) {
    if (track_no <= 0 || track_model_ == nullptr) {
        return;
    }

    for (int albumRow = 0; albumRow < track_model_->rowCount(); ++albumRow) {
        auto* albumItem = track_model_->item(albumRow, 0);
        if (albumItem == nullptr) {
            continue;
        }

        for (int row = 0; row < albumItem->rowCount(); ++row) {
            auto* durationItem = albumItem->child(row, 2);
            if (durationItem == nullptr) {
                continue;
            }

            const auto data = durationItem->data(Qt::UserRole + 1);
            if (!data.isValid()) {
                continue;
            }

            const auto entity = data.value<PlayListEntity>();
            if (static_cast<int>(entity.track) != track_no) {
                continue;
            }

            const auto index = albumItem->child(row, 0)->index();
            ui_->trackView->setCurrentIndex(index);
            ui_->trackView->scrollTo(index, QAbstractItemView::PositionAtCenter);
            return;
        }
    }
}

void MusicbrainzEditPage::updateFetchProgressText(const QString& state) {
    if (fetch_progress_bar_ == nullptr) {
        return;
    }

    fetch_progress_bar_->setFormat(tr("%1 (%2/%3) album")
        .arg(state)
        .arg(completed_albums_)
        .arg(total_albums_));
}

QCoro::Task<> MusicbrainzEditPage::startFetchMusicBrainzRecording() {
    is_fetching_ = true;
    QMap<QString, QList<PlayListEntity>> album_unique_map;
    for (const auto& entity : entities_) {
        album_unique_map[entity.album].append(entity);
    }

    completed_albums_ = 0;
    total_albums_ = album_unique_map.size();
    completed_recordings_ = 0;
    total_recordings_ = 0;
    completed_releases_ = 0;
    total_releases_ = 0;
    if (fetch_progress_bar_ != nullptr) {
        fetch_progress_bar_->setRange(0, total_albums_ > 0 ? total_albums_ : 1);
        fetch_progress_bar_->setValue(0);
        updateFetchProgressText(tr("Fetching"));
    }

    try {
        QList<PendingReleaseLookup> pending_lookups;
        QSet<QString> pending_release_ids;
        for (const auto& list_entities : album_unique_map) {
            auto candidate_releases = co_await fetchCandidateReleases(list_entities);
            if (!candidate_releases.isEmpty()) {
                pending_release_ids += releaseIds(candidate_releases);
                pending_lookups.append({ list_entities, candidate_releases });
            }
            ++completed_albums_;
            total_releases_ = pending_release_ids.size();
            updateFetchProgressText(tr("Fetching"));
        }
        total_releases_ = pending_release_ids.size();
        XAMP_LOG_DEBUG("Total {} releases", total_releases_);

        if (pending_lookups.isEmpty()) {
            is_fetching_ = false;
            completed_albums_ = 0;
            if (fetch_progress_bar_ != nullptr) {
                fetch_progress_bar_->setRange(0, total_albums_ > 0 ? total_albums_ : 1);
                fetch_progress_bar_->setValue(0);
                updateFetchProgressText(tr("No release found"));
            }
            co_return;
        }

        completed_albums_ = 0;
        completed_releases_ = 0;
        if (fetch_progress_bar_ != nullptr) {
            fetch_progress_bar_->setRange(0, total_albums_ > 0 ? total_albums_ : 1);
            fetch_progress_bar_->setValue(0);
            updateFetchProgressText(tr("Fetching"));
        }

        QSet<QString> completed_album_keys;
        QSet<QString> fetched_release_ids;
        QHash<QString, QList<musicbrain::TrackInfo>> release_track_cache;
        QHash<QString, QPixmap> release_cover_cache;
        for (const auto& lookup : pending_lookups) {
            completed_album_keys.insert(lookup.entities.isEmpty() ? QString() : lookup.entities.front().album);
            completed_albums_ = std::min(static_cast<int>(completed_album_keys.size()), total_albums_);
            if (fetch_progress_bar_ != nullptr) {
                fetch_progress_bar_->setValue(completed_albums_);
            }
            updateFetchProgressText(tr("Fetching"));
            co_await fetchMusicBrainzRelease(lookup.entities,
                lookup.candidateReleases,
                fetched_release_ids,
                release_track_cache,
                release_cover_cache);
        }
    }
    catch (...) {
    }

    is_fetching_ = false;
    if (fetch_progress_bar_ != nullptr) {
        completed_albums_ = total_albums_;
        completed_recordings_ = total_recordings_;
        completed_releases_ = total_releases_;
        fetch_progress_bar_->setRange(0, total_albums_ > 0 ? total_albums_ : 1);
        fetch_progress_bar_->setValue(total_albums_ > 0 ? total_albums_ : 1);
        updateFetchProgressText(tr("Completed"));
    }
    co_return;
}

QCoro::Task<std::optional<QByteArray>> MusicbrainzEditPage::tryFetchCoverArt(const QString& tag,
    const QString& release_id,
    size_t size) {
    const auto url = (size > 0)
        ? qFormat("https://coverartarchive.org/%1/%2/front-%3").arg(tag).arg(release_id).arg(size)
        : qFormat("https://coverartarchive.org/%1/%2/front").arg(tag).arg(release_id);
    http_client_.setUrl(url);
    http_client_.setHeader("Accept"_str, "image/*"_str);

    auto img = co_await http_client_.download();
    if (!img.isEmpty()) {
        co_return img;
    }
    co_return std::nullopt;
}

QCoro::Task<std::optional<QByteArray>> MusicbrainzEditPage::fetchCoverArtByUrl(const QString& tag,
    const QString& release_id,
    size_t prefer_size) {
    std::optional<QByteArray> b;
    if (prefer_size > 0) {
        auto result = co_await tryFetchCoverArt(tag, release_id, prefer_size);
        if (result.has_value()) {
            b = result.value();
        }
        else {
            result = co_await tryFetchCoverArt(tag, release_id, 500);
            if (result.has_value()) {
                b = result.value();
            }
        }
    }
    else {
        auto result = co_await tryFetchCoverArt(tag, release_id, 0);
        if (result.has_value()) {
            b = result.value();
        }
    }
    if (!b.has_value()) {
        co_return std::nullopt;
    }
    co_return b;
}

QCoro::Task<QList<musicbrain::Release>> MusicbrainzEditPage::fetchCandidateReleases(const QList<PlayListEntity>& entities) {
    QList<musicbrain::Release> candidate_releases;
    const auto query = buildMusicBrainzReleaseQuery(entities);
    if (query.isEmpty()) {
        co_return candidate_releases;
    }

    http_client_.setUrl("https://musicbrainz.org/ws/2/release"_str);
    http_client_.param("query"_str, query);
    http_client_.param("fmt"_str, "json"_str);
    http_client_.param("limit"_str, 10);
    auto content = co_await http_client_.get();

    auto releases = musicbrain::parseReleaseList(content);
    if (!releases.has_value() || releases->isEmpty()) {
        co_return candidate_releases;
    }
    candidate_releases = releases.value();
    co_return candidate_releases;
}

QCoro::Task<bool> MusicbrainzEditPage::fetchMusicBrainzRelease(const QList<PlayListEntity>& entities,
    const QList<musicbrain::Release>& candidate_releases,
    QSet<QString>& fetched_release_ids,
    QHash<QString, QList<musicbrain::TrackInfo>>& release_track_cache,
    QHash<QString, QPixmap>& release_cover_cache) {
    static constexpr size_t kDefaultSize = 500;

    bool found = false;
    if (entities.isEmpty()) {
        co_return found;
    }
    const auto album_meta = makeFileMeta(entities.front(), entities.count());

    for (const auto& r : candidate_releases) {
        if (r.id.isEmpty()) {
            continue;
        }

        QList<musicbrain::TrackInfo> tracks;
        QPixmap cover_art;
        if (fetched_release_ids.contains(r.id)) {
            if (!release_track_cache.contains(r.id)) {
                continue;
            }
            tracks = release_track_cache.value(r.id);
            cover_art = release_cover_cache.value(r.id);
        }
        else {
            http_client_.setUrl("https://musicbrainz.org/ws/2/release/"_str + r.id);
            http_client_.param("inc"_str, "release-groups+recordings+media+artist-credits"_str);
            http_client_.param("fmt"_str, "json"_str);
            http_client_.param("client"_str, "J0OsCydP14"_str);

            const auto content = co_await http_client_.get();
            QList<musicbrain::Release> current_release;
            current_release.append(r);
            auto parsed_tracks = musicbrain::parseReleaseTracklist(content.toUtf8(), current_release);
            fetched_release_ids.insert(r.id);
            if (!parsed_tracks.has_value()) {
                completed_releases_ = std::min(completed_releases_ + 1, total_releases_);
                continue;
            }

            const auto cover_art_bytes = co_await fetchCoverArtByUrl("release"_str, r.id, kDefaultSize);
            if (cover_art_bytes.has_value() && !cover_art_bytes->isEmpty()) {
                cover_art.loadFromData(cover_art_bytes.value());
            }

            tracks = parsed_tracks.value();
            release_track_cache.insert(r.id, tracks);
            release_cover_cache.insert(r.id, cover_art);
            total_recordings_ += uniqueRecordingCount(tracks);

            completed_releases_ = std::min(completed_releases_ + 1, total_releases_);
        }

        const auto release_score = musicbrain::compareToRelease(album_meta, r);
        QSet<QString> appended_recordings;
        for (const auto& track : tracks) {
            const auto recording_key = track.recordingId.isEmpty()
                ? qFormat("%1|%2|%3").arg(track.title).arg(track.trackNo).arg(track.lengthMs)
                : track.recordingId;
            if (appended_recordings.contains(recording_key)) {
                continue;
            }
            appended_recordings.insert(recording_key);

            MusicBrainzRecording music_brainz_recording;
            music_brainz_recording.release_id = r.id;
            music_brainz_recording.root_recording = rootRecordingFromTrack(track, r);
            music_brainz_recording.similarity = release_score;
            music_brainz_recording.title = qFormat("%1 (%2 %3) %4%")
                .arg(r.title)
                .arg(r.status)
                .arg(r.country)
                .arg(QString::number(std::round(music_brainz_recording.similarity * 100.0), 'f', 0));
            music_brainz_recording.cover_art = cover_art;
            music_brainz_recording.tracks = QList<musicbrain::TrackInfo>{ track };

            MusicBrainzAlbum partial_album;
            partial_album.recordings.append(music_brainz_recording);
            appendMusicBrainzAlbum(partial_album);
            found = true;
            completed_recordings_ = std::min(completed_recordings_ + 1, total_recordings_);
        }
    }
    co_return found;
}

void MusicbrainzEditPage::setTagPreview(const std::optional<PlayListEntity>& entity,
    const std::optional<musicbrain::TrackInfo>& track,
    const QString& album) {
    struct TagRow {
        QString tag;
        QString oldValue;
        QString newValue;
    };

    QList<TagRow> rows;
    auto addRow = [&rows](const QString& tag, const QString& oldValue, const QString& newValue, bool always = true) {
        if (!always && oldValue.isEmpty() && newValue.isEmpty()) {
            return;
        }
        rows.append({ tag, oldValue, newValue });
    };

    const auto release = track && !track->releases.isEmpty()
        ? std::optional<musicbrain::Release>{ track->releases.front() }
        : std::optional<musicbrain::Release>{};
    const auto newAlbum = release && !release->title.isEmpty() ? release->title : album;
    const auto newArtist = track ? artistText(*track) : QString();

    addRow("tracknumber"_str, entity ? QString::number(entity->track) : QString(), track ? QString::number(track->trackNo) : QString());
    addRow("title"_str, entity ? entity->title : QString(), track ? track->title : QString());
    addRow("album"_str, entity ? entity->album : QString(), track ? newAlbum : QString());
    addRow("artist"_str, entity ? entity->artist : QString(), newArtist);
    addRow("length"_str,
        entity ? formatDuration(entity->duration) : QString(),
        track && track->lengthMs > 0 ? formatDuration(track->lengthMs / 1000.0) : QString());

    addRow("discnumber"_str, QString(), track && track->disc > 0 ? QString::number(track->disc) : QString(), false);
    addRow("musicbrainz_trackid"_str, QString(), track ? track->id : QString(), false);
    addRow("musicbrainz_recordingid"_str, QString(), track ? track->recordingId : QString(), false);

    if (release.has_value()) {
        addRow("albumartist"_str, QString(), artistCreditText(release->artistCredits), false);
        addRow("musicbrainz_albumartistid"_str, QString(), artistCreditIds(release->artistCredits), false);
        addRow("musicbrainz_albumid"_str, QString(), release->id, false);
        addRow("musicbrainz_releasegroupid"_str, QString(), release->releaseGroup.id, false);
        addRow("releasetype"_str, QString(), releaseTypeText(release->releaseGroup), false);
        addRow("date"_str, entity && entity->year > 0 ? QString::number(entity->year) : QString(), releaseDateText(*release), false);
        addRow("releasecountry"_str, QString(), releaseCountryText(*release), false);
        addRow("musicbrainz_albumstatus"_str, QString(), release->status, false);
        addRow("barcode"_str, QString(), release->barcode, false);
        addRow("media"_str, QString(), release->mediaFormats.join(";"_str), false);
        addRow("script"_str, QString(), release->textRep.script, false);
        addRow("language"_str, QString(), release->textRep.language, false);
    }

    tag_model_->setRowCount(rows.size());
    for (int row = 0; row < rows.size(); ++row) {
        auto* tag_item = new QStandardItem(rows[row].tag);
        auto* old_item = new QStandardItem(rows[row].oldValue);
        auto* new_item = new QStandardItem(rows[row].newValue);
        tag_item->setEditable(false);
        old_item->setEditable(false);
        tag_model_->setItem(row, 0, tag_item);
        tag_model_->setItem(row, 1, old_item);
        tag_model_->setItem(row, 2, new_item);
    }
}

void MusicbrainzEditPage::writeSelectedTag() {
    if (!selected_entity_ || !selected_track_) {
        return;
    }

    auto entity = *selected_entity_;
    const auto& track = *selected_track_;
    const auto artist = artistText(track);

    if (entity.is_cue_file || !entity.isFilePath()) {
        XMessageBox::showWarning(tr("This file can not be written."));
        return;
    }

    if (XMessageBox::showYesOrNo(tr("Do you want write tag?")) != QDialogButtonBox::Yes) {
        return;
    }

    try {
        TagIO tag_io;
        tag_io.open(entity.file_path.toStdWString());
        tag_io.writeTrack(static_cast<uint32_t>(track.trackNo));
        tag_io.writeTitle(track.title);
        tag_io.writeAlbum(selected_album_);
        tag_io.writeArtist(artist);
    }
    catch (...) {
        XMessageBox::showError(tr("Failure to write tag!"));
        return;
    }

    try {
        qDaoFacade.music_dao.updateMusicTitle(entity.music_id, track.title);
        qDaoFacade.album_dao.updateAlbum(entity.album_id, selected_album_);
        qDaoFacade.artist_dao.updateArtist(entity.artist_id, artist);
    }
    catch (...) {
        XMessageBox::showError(tr("Failure to update database!"));
        return;
    }

    entity.track = static_cast<uint32_t>(track.trackNo);
    entity.title = track.title;
    entity.album = selected_album_;
    entity.artist = artist;
    selected_entity_ = entity;
    setTagPreview(selected_entity_, selected_track_, selected_album_);

    XMessageBox::showInformation(tr("Write tag successfully!"));
}

void MusicbrainzEditPage::closeEvent(QCloseEvent* event) {
    if (is_fetching_) {
        event->ignore();
        return;
    }
    QFrame::closeEvent(event);
}

void MusicbrainzEditPage::onThemeChangedFinished(ThemeColor theme_color) {    
}

void MusicbrainzEditPage::onRetranslateUi() {
    ui_->retranslateUi(this);
    if (fetch_progress_bar_ != nullptr) {
        updateFetchProgressText(is_fetching_ ? tr("Fetching") : tr("Completed"));
    }
    if (write_tag_button_ != nullptr) {
        write_tag_button_->setText(tr("Write Tags"));
    }
}
