#include <QStandardItemModel>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QEventLoop>
#include <QHash>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <ui_musicbrainzeditpage.h>
#include <widget/dao/dbfacade.h>
#include <widget/musicbrainzeditpage.h>
#include <widget/util/image_util.h>
#include <widget/util/str_util.h>
#include <widget/util/tag_util.h>
#include <widget/util/ui_util.h>
#include <widget/xmessagebox.h>

#include <algorithm>
#include <cmath>

namespace {
    constexpr double kMinVisibleAlbumScore = 0.55;
    constexpr int kReleaseIdRole = Qt::UserRole + 1;
    constexpr int kTrackInfoRole = Qt::UserRole + 2;
    constexpr int kAlbumTitleRole = Qt::UserRole + 3;
    constexpr int kMatchedEntityRole = Qt::UserRole + 4;
    constexpr int kDiscNumberRole = Qt::UserRole + 5;

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

    QString escapeLuceneQuery(const QString& value) {
        QString escaped;
        escaped.reserve(value.size() * 2);
        for (const auto ch : value) {
            switch (ch.unicode()) {
            case '+':
            case '-':
            case '&':
            case '|':
            case '!':
            case '(':
            case ')':
            case '{':
            case '}':
            case '[':
            case ']':
            case '^':
            case '"':
            case '~':
            case '*':
            case '?':
            case ':':
            case '\\':
            case '/':
                escaped.append(QLatin1Char('\\'));
                break;
            default:
                break;
            }
            escaped.append(ch);
        }
        return escaped;
    }

    QString luceneField(const QString& field, const QString& value) {
        if (value.isEmpty()) {
            return QString();
        }
        return qFormat("%1:(%2)"_str)
            .arg(field, escapeLuceneQuery(value));
    }

    QString buildMusicBrainzReleaseQuery(const QList<PlayListEntity>& entities) {
        QStringList parts;
        if (entities.isEmpty()) {
            return QString();
        }
        const auto& entity = entities.front();
        if (!entity.artist.isEmpty()) {
            parts.append(luceneField("artist"_str, entity.artist));
        }
        if (!entity.album.isEmpty()) {
            parts.append(luceneField("release"_str, entity.album));
        }
        if (!entities.isEmpty()) {
            parts.append(luceneField("tracks"_str, QString::number(entities.count())));
        }
        if (entity.year > 0) {
            parts.append(luceneField("date"_str, QString::number(entity.year)));
        }
        return parts.join(" "_str);
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

    void appendDiscRowSorted(QStandardItem* album_item,
        const QList<QStandardItem*>& row_items,
        int disc_no) {
        auto insertRow = album_item->rowCount();
        for (int row = 0; row < album_item->rowCount(); ++row) {
            auto* duration_item = album_item->child(row, 3);
            const auto itemDiscNo = duration_item != nullptr ? duration_item->data(kDiscNumberRole).toInt() : 0;
            if (disc_no > 0 && itemDiscNo > disc_no) {
                insertRow = row;
                break;
            }
        }
        album_item->insertRow(insertRow, row_items);
    }

    int totalDiscCount(const MusicBrainzRecording& recording) {
        auto total_discs = 0;
        if (const auto release = releaseForRecording(recording); release.has_value()) {
            total_discs = release->mediaFormats.size();
        }
        for (const auto& track : recording.tracks) {
            total_discs = std::max(total_discs, track.disc);
        }
        return total_discs;
    }

    QString discDisplayTitle(const MusicBrainzRecording& recording, int disc_no) {
        auto format = QStringLiteral("CD");
        if (const auto release = releaseForRecording(recording); release.has_value()) {
            const auto index = disc_no - 1;
            if (index >= 0 && index < release->mediaFormats.size() && !release->mediaFormats[index].isEmpty()) {
                format = release->mediaFormats[index];
            }
        }
        return qFormat("%1 %2").arg(format).arg(disc_no);
    }

    void collectTrackDurationItems(QStandardItem* parent_item, QList<QStandardItem*>& items) {
        if (parent_item == nullptr) {
            return;
        }

        for (int row = 0; row < parent_item->rowCount(); ++row) {
            auto* duration_item = parent_item->child(row, 3);
            if (duration_item != nullptr && duration_item->data(kTrackInfoRole).isValid()) {
                items.append(duration_item);
                continue;
            }
            collectTrackDurationItems(parent_item->child(row, 0), items);
        }
    }

    QStandardItem* findDiscItem(QStandardItem* album_item, int disc_no) {
        if (album_item == nullptr) {
            return nullptr;
        }

        for (int row = 0; row < album_item->rowCount(); ++row) {
            auto* duration_item = album_item->child(row, 3);
            if (duration_item != nullptr && duration_item->data(kDiscNumberRole).toInt() == disc_no) {
                return album_item->child(row, 0);
            }
        }
        return nullptr;
    }

    std::optional<PlayListEntity> matchedEntityForItem(const QStandardItem* item) {
        if (item == nullptr) {
            return std::nullopt;
        }

        const auto data = item->data(kMatchedEntityRole);
        if (!data.isValid()) {
            return std::nullopt;
        }
        return data.value<PlayListEntity>();
    }

    QStandardItem* writeScopeItemForIndex(QStandardItemModel* model, QModelIndex index) {
        if (model == nullptr || !index.isValid()) {
            return nullptr;
        }

        QModelIndex albumIndex;
        while (index.isValid()) {
            const auto durationIndex = index.siblingAtColumn(3);
            const auto isDiscNode = durationIndex.data(kDiscNumberRole).toInt() > 0;
            if (isDiscNode) {
                return model->itemFromIndex(index.siblingAtColumn(0));
            }

            albumIndex = index;
            index = index.parent();
        }

        return albumIndex.isValid()
            ? model->itemFromIndex(albumIndex.siblingAtColumn(0))
            : nullptr;
    }

    QString writeScopeText(const QStandardItem* item) {
        if (item == nullptr) {
            return QObject::tr("album");
        }

        const auto durationItem = item->parent() != nullptr
            ? item->parent()->child(item->row(), 3)
            : nullptr;
        if (durationItem != nullptr && durationItem->data(kDiscNumberRole).toInt() > 0) {
            return QObject::tr("disc");
        }
        return QObject::tr("album");
    }

    QString candidateKey(const MusicBrainzRecording& recording) {
        const auto track = recording.tracks.isEmpty()
            ? musicbrain::TrackInfo{}
            : recording.tracks.front();
        const auto recordingKey = recording.root_recording.id.isEmpty()
            ? qFormat("%1|%2|%3")
                .arg(recording.root_recording.title)
                .arg(track.disc)
                .arg(track.trackNo)
            : recording.root_recording.id;
        return QStringLiteral("%1|%2|%3|%4")
            .arg(recordingKey)
            .arg(track.disc)
            .arg(track.trackNo)
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

    write_tag_button_ = new QPushButton(tr("Write Selected Track"), this);
    write_tag_button_->setEnabled(false);
    write_tag_button_->setToolTip(tr("Write tags only to the selected track."));

    write_album_tags_button_ = new QPushButton(tr("Write Entire Album"), this);
    write_album_tags_button_->setEnabled(false);
    write_album_tags_button_->setToolTip(tr("Write tags to every matched track in the selected album."));

    fetch_progress_bar_ = new QProgressBar(this);
    fetch_progress_bar_->setTextVisible(true);
    fetch_progress_bar_->setRange(0, 1);
    fetch_progress_bar_->setValue(0);
    fetch_progress_bar_->setFormat(tr("Fetching MusicBrainz releases (0/0)"));
    fetch_progress_bar_->setMinimumWidth(260);

    auto* button_layout = new QHBoxLayout();
    button_layout->addWidget(fetch_progress_bar_, 1);
    button_layout->addStretch();
    button_layout->addWidget(write_tag_button_);
    button_layout->addWidget(write_album_tags_button_);
    ui_->verticalLayout_2->addLayout(button_layout);

    (void)QObject::connect(write_tag_button_, &QPushButton::clicked, this, &MusicbrainzEditPage::writeSelectedTag);
    (void)QObject::connect(write_album_tags_button_, &QPushButton::clicked, this, &MusicbrainzEditPage::writeSelectedAlbumTags);

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

    auto sorted_entities = entities;
    std::stable_sort(sorted_entities.begin(), sorted_entities.end(), [](const auto& left, const auto& right) {
        if (left.track == right.track) {
            return false;
        }
        if (left.track == 0) {
            return false;
        }
        if (right.track == 0) {
            return true;
        }
        return left.track < right.track;
    });

    entities_ = sorted_entities;
    metas_.clear();
    recording_list_.clear();
    track_model_->clear();
    album_track_model_->clear();
    tag_model_->clear();
    cover_art_map_.clear();
    cover_art_size_map_.clear();

    auto f = qTheme.defaultFont();
    f.setPointSize(qTheme.fontSize(9));
    setFont(f);
    ui_->trackView->setFont(f);
    ui_->albumRecordingView->setFont(f);
    ui_->tagTableView->setFont(f);
    ui_->groupBox->setFont(f);
    ui_->originalCoverTitle->setFont(f);
    ui_->originalCoverArt->setFont(f);
    ui_->originalCoverArtSize->setFont(f);
    ui_->newCoverTitle->setFont(f);
    ui_->newCoverArt->setFont(f);
    ui_->newCoverArtSize->setFont(f);
    if (fetch_progress_bar_ != nullptr) {
        fetch_progress_bar_->setFont(f);
    }
    if (write_tag_button_ != nullptr) {
        write_tag_button_->setFont(f);
    }
    if (write_album_tags_button_ != nullptr) {
        write_album_tags_button_->setFont(f);
    }

    QStringList headers;
    headers << tr("Track") << tr("Title") << tr("Duration");
    track_model_->setColumnCount(headers.size());
    track_model_->setHorizontalHeaderLabels(headers);
    ui_->trackView->setModel(track_model_);
    ui_->trackView->setRootIsDecorated(true);
    ui_->trackView->setAllColumnsShowFocus(true);

    auto* album_item = new QStandardItem(sorted_entities.front().album);

    QList<QStandardItem*> top_row;
    top_row << album_item
        << new QStandardItem(QString())
        << new QStandardItem(QString());
    track_model_->appendRow(top_row);

    for (const auto& entity : sorted_entities) {
        auto* child1 = new QStandardItem(QString::number(entity.track));
        auto* child2 = new QStandardItem(entity.title);
        auto* child3 = new QStandardItem(formatDuration(entity.duration));
        child3->setData(QVariant::fromValue(entity), Qt::UserRole + 1);
        QList<QStandardItem*> row_items;
        row_items << child1 << child2 << child3;
        album_item->appendRow(row_items);
        metas_.append(makeFileMeta(entity, sorted_entities.count()));
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
    ui_->albumRecordingView->setIconSize(QSize(18, 18));

    (void)QObject::connect(ui_->albumRecordingView, &QTreeView::clicked,
        [this](const auto& index) {
        if (!index.isValid()) {
            return;
        }

        auto idx = index.siblingAtColumn(3);
        updateNewCoverArt(idx.data(kReleaseIdRole).toString());
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
    setTagPreview(sorted_entities.front(), std::nullopt, QString());
    updateOriginalCoverArt(sorted_entities.front());
    updateNewCoverArt(QString());

    (void)QObject::connect(ui_->albumRecordingView, &QTreeView::clicked,
        [this](const auto& index) {
        auto idx = index.siblingAtColumn(3);
        if (!index.isValid())
           return;
        auto data = idx.data(kTrackInfoRole);
        if (!data.isValid()) {
           selected_track_.reset();
           selected_album_.clear();
           data = idx.data(kAlbumTitleRole);
           if (data.isValid()) {
               selected_album_ = data.template value<QString>();
           }
           selected_release_id_ = idx.data(kReleaseIdRole).toString();
           std::optional<PlayListEntity> album_entity;
           while (idx.parent().isValid()) {
               idx = idx.parent();
           }
           auto* album_item = album_track_model_->itemFromIndex(idx.siblingAtColumn(0));
           if (album_item != nullptr) {
               QList<QStandardItem*> track_items;
               collectTrackDurationItems(album_item, track_items);
               for (auto* duration_item : track_items) {
                   album_entity = matchedEntityForItem(duration_item);
                   if (album_entity.has_value()) {
                       break;
                   }
               }
           }
           updateOriginalCoverArt(album_entity);
           updateWriteTagButtons();
           return;
        }
        auto tracks = data.template value<musicbrain::TrackInfo>();
        data = idx.data(kAlbumTitleRole);
        if (!data.isValid())
            return;
        auto album = data.template value<QString>();
        auto entity = matchedEntityForItem(album_track_model_->itemFromIndex(idx));
        if (!entity.has_value()) {
            entity = entityForExactTrack(tracks.trackNo);
        }
        if (entity.has_value()) {
            selectTrackViewEntity(entity->music_id);
        }
        setTagPreview(entity, tracks, album);
        selected_entity_ = entity;
        selected_track_ = tracks;
        selected_album_ = album;
        selected_release_id_ = idx.data(kReleaseIdRole).toString();
        updateOriginalCoverArt(selected_entity_);
        updateNewCoverArt(selected_release_id_);
        updateWriteTagButtons();
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
        selected_release_id_.clear();
        ui_->albumRecordingView->clearSelection();
        ui_->albumRecordingView->setCurrentIndex(QModelIndex());
        setTagPreview(entity, std::nullopt, QString());
        updateOriginalCoverArt(entity);
        updateNewCoverArt(QString());
        updateWriteTagButtons();
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
        QHash<int, int> matchedFileIndexes;
        for (const auto& match : candidate.matches) {
            trackSimilarities.insert(match.trackIndex, match.similarity);
            if (match.trackIndex >= 0 && match.fileIndex >= 0 && match.fileIndex < entities_.size()) {
                matchedFileIndexes.insert(match.trackIndex, match.fileIndex);
            }
        }

        const auto albumTitle = albumDisplayTitle(recording);
        const auto releaseTitle = releaseDisplayTitle(recording);
        const auto albumKey = recording.release_id.isEmpty() ? releaseTitle.toCaseFolded() : recording.release_id;

        auto* albumItem = albumItems.value(albumKey, nullptr);
        if (albumItem == nullptr) {
            albumItem = new QStandardItem(releaseTitle);
            albumItem->setIcon(qTheme.fontIcon(Glyphs::ICON_CD));
            QList<QStandardItem*> albumRow;
            albumRow << albumItem
                << new QStandardItem(similarityText(candidate.albumScore))
                << new QStandardItem(QString())
                << new QStandardItem(QString());
            albumRow[3]->setData(recording.release_id, kReleaseIdRole);
            albumRow[3]->setData(albumTitle, kAlbumTitleRole);
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

        const auto showDiscNodes = totalDiscCount(recording) > 1;
        QHash<int, QStandardItem*> discItems;
        for (int i = 0; i < recording.tracks.size(); ++i) {
            const auto& track = recording.tracks[i];
            auto* parentItem = albumItem;
            if (showDiscNodes) {
                const auto discNo = track.disc > 0 ? track.disc : 1;
                parentItem = discItems.value(discNo, nullptr);
                if (parentItem == nullptr) {
                    parentItem = findDiscItem(albumItem, discNo);
                }
                if (parentItem == nullptr) {
                    parentItem = new QStandardItem(discDisplayTitle(recording, discNo));
                    parentItem->setIcon(qTheme.fontIcon(Glyphs::ICON_CD));
                    auto* discScoreItem = new QStandardItem(QString());
                    auto* discTrackItem = new QStandardItem(QString());
                    auto* discDurationItem = new QStandardItem(QString());
                    discDurationItem->setData(recording.release_id, kReleaseIdRole);
                    discDurationItem->setData(albumTitle, kAlbumTitleRole);
                    discDurationItem->setData(discNo, kDiscNumberRole);
                    appendDiscRowSorted(albumItem, { parentItem, discScoreItem, discTrackItem, discDurationItem }, discNo);
                }
                discItems.insert(discNo, parentItem);
            }

            auto* trackItem = new QStandardItem(track.title);
            auto* trackScoreItem = new QStandardItem(trackSimilarities.contains(i) ? similarityText(trackSimilarities.value(i)) : QString());
            auto* trackNumberItem = new QStandardItem(QString::number(track.trackNo));
            auto* trackDurationItem = new QStandardItem(track.lengthMs > 0 ? formatDuration(track.lengthMs / 1000.0) : QString());
            trackDurationItem->setData(recording.release_id, kReleaseIdRole);
            trackDurationItem->setData(QVariant::fromValue(track), kTrackInfoRole);
            trackDurationItem->setData(albumTitle, kAlbumTitleRole);
            if (const auto itr = matchedFileIndexes.find(i); itr != matchedFileIndexes.end()) {
                trackDurationItem->setData(QVariant::fromValue(entities_[itr.value()]), kMatchedEntityRole);
            }
            appendTrackRowSorted(parentItem, { trackItem, trackScoreItem, trackNumberItem, trackDurationItem }, track.trackNo);
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

std::optional<PlayListEntity> MusicbrainzEditPage::entityForExactTrack(int track_no) const {
    for (const auto& entity : entities_) {
        if (static_cast<int>(entity.track) == track_no) {
            return entity;
        }
    }
    return std::nullopt;
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

void MusicbrainzEditPage::selectTrackViewEntity(int32_t music_id) {
    if (music_id <= 0 || track_model_ == nullptr) {
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
            if (entity.music_id != music_id) {
                continue;
            }

            const auto index = albumItem->child(row, 0)->index();
            ui_->trackView->setCurrentIndex(index);
            ui_->trackView->scrollTo(index, QAbstractItemView::PositionAtCenter);
            return;
        }
    }
}

void MusicbrainzEditPage::updateFetchProgressText(const QString& state, bool use_release_progress) {
    if (fetch_progress_bar_ == nullptr) {
        return;
    }

    if (use_release_progress) {
        fetch_progress_bar_->setFormat(tr("%1 MusicBrainz releases (%2/%3)")
            .arg(state)
            .arg(completed_releases_)
            .arg(total_releases_));
        return;
    }

    fetch_progress_bar_->setFormat(tr("%1 albums (%2/%3)")
        .arg(state)
        .arg(completed_albums_)
        .arg(total_albums_));
}

void MusicbrainzEditPage::updateWriteProgressText(const QString& state) {
    if (fetch_progress_bar_ == nullptr) {
        return;
    }

    fetch_progress_bar_->setFormat(tr("%1 (%2/%3) tracks")
        .arg(state)
        .arg(completed_writes_)
        .arg(total_writes_));
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
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
            if (fetch_progress_bar_ != nullptr) {
                fetch_progress_bar_->setValue(completed_albums_);
            }
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
            fetch_progress_bar_->setRange(0, total_releases_ > 0 ? total_releases_ : 1);
            fetch_progress_bar_->setValue(0);
            updateFetchProgressText(tr("Fetching"), true);
        }

        QSet<QString> completed_album_keys;
        QSet<QString> fetched_release_ids;
        QHash<QString, QList<musicbrain::TrackInfo>> release_track_cache;
        QHash<QString, QPixmap> release_cover_cache;
        QHash<QString, size_t> release_cover_size_cache;
        for (const auto& lookup : pending_lookups) {
            updateFetchProgressText(tr("Fetching"), true);
            co_await fetchMusicBrainzRelease(lookup.entities,
                lookup.candidateReleases,
                fetched_release_ids,
                release_track_cache,
                release_cover_cache,
                release_cover_size_cache);

            completed_album_keys.insert(lookup.entities.isEmpty() ? QString() : lookup.entities.front().album);
            completed_albums_ = std::min(static_cast<int>(completed_album_keys.size()), total_albums_);
            updateFetchProgressText(tr("Fetching"), true);
        }
    }
    catch (...) {
    }

    is_fetching_ = false;
    if (fetch_progress_bar_ != nullptr) {
        completed_albums_ = total_albums_;
        completed_recordings_ = total_recordings_;
        completed_releases_ = total_releases_;
        if (total_releases_ > 0) {
            fetch_progress_bar_->setRange(0, total_releases_);
            fetch_progress_bar_->setValue(total_releases_);
            updateFetchProgressText(tr("Completed"), true);
        }
        else {
            fetch_progress_bar_->setRange(0, total_albums_ > 0 ? total_albums_ : 1);
            fetch_progress_bar_->setValue(total_albums_ > 0 ? total_albums_ : 1);
            updateFetchProgressText(tr("Completed"));
        }
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
    QHash<QString, QPixmap>& release_cover_cache,
    QHash<QString, size_t>& release_cover_size_cache) {
    static constexpr size_t kDefaultSize = 1200;

    bool found = false;
    if (entities.isEmpty()) {
        co_return found;
    }
    const auto album_meta = makeFileMeta(entities.front(), entities.count());
    auto update_release_progress = [this] {
        if (fetch_progress_bar_ == nullptr) {
            return;
        }
        fetch_progress_bar_->setValue(completed_releases_);
        updateFetchProgressText(tr("Fetching"), true);
        };

    for (const auto& r : candidate_releases) {
        if (r.id.isEmpty()) {
            continue;
        }

        QList<musicbrain::TrackInfo> tracks;
        QPixmap cover_art;
        size_t cover_art_size = 0;
        if (fetched_release_ids.contains(r.id)) {
            if (!release_track_cache.contains(r.id)) {
                continue;
            }
            tracks = release_track_cache.value(r.id);
            cover_art = release_cover_cache.value(r.id);
            cover_art_size = release_cover_size_cache.value(r.id, 0);
            cover_art_size_map_.insert(r.id, cover_art_size);
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
                update_release_progress();
                continue;
            }

            const auto cover_art_bytes = co_await fetchCoverArtByUrl("release"_str, r.id, kDefaultSize);
            if (cover_art_bytes.has_value() && !cover_art_bytes->isEmpty()) {
                cover_art.loadFromData(cover_art_bytes.value());
                cover_art_size = static_cast<size_t>(cover_art_bytes->size());
            }

            tracks = parsed_tracks.value();
            release_track_cache.insert(r.id, tracks);
            release_cover_cache.insert(r.id, cover_art);
            release_cover_size_cache.insert(r.id, cover_art_size);
            cover_art_size_map_.insert(r.id, cover_art_size);
            total_recordings_ += uniqueRecordingCount(tracks);

            completed_releases_ = std::min(completed_releases_ + 1, total_releases_);
            update_release_progress();
        }

        const auto release_score = musicbrain::compareToRelease(album_meta, r);
        QSet<QString> appended_recordings;
        for (const auto& track : tracks) {
            const auto recording_key = track.recordingId.isEmpty()
                ? qFormat("%1|%2|%3|%4").arg(track.title).arg(track.disc).arg(track.trackNo).arg(track.lengthMs)
                : qFormat("%1|%2|%3").arg(track.recordingId).arg(track.disc).arg(track.trackNo);
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

void MusicbrainzEditPage::updateWriteTagButtons() {
    if (is_writing_) {
        if (write_tag_button_ != nullptr) {
            write_tag_button_->setEnabled(false);
        }
        if (write_album_tags_button_ != nullptr) {
            write_album_tags_button_->setEnabled(false);
        }
        return;
    }

    if (write_tag_button_ != nullptr) {
        write_tag_button_->setEnabled(selected_entity_.has_value()
            && selected_track_.has_value()
            && !selected_entity_->is_cue_file
            && selected_entity_->isFilePath());
    }

    if (write_album_tags_button_ == nullptr) {
        return;
    }

    auto index = ui_->albumRecordingView->currentIndex();
    if (!index.isValid()) {
        write_album_tags_button_->setEnabled(false);
        return;
    }

    auto* scope_item = writeScopeItemForIndex(album_track_model_, index);
    if (scope_item == nullptr) {
        write_album_tags_button_->setEnabled(false);
        return;
    }

    QList<QStandardItem*> track_items;
    collectTrackDurationItems(scope_item, track_items);

    auto has_writable_track = false;
    for (auto* duration_item : track_items) {
        auto entity = matchedEntityForItem(duration_item);
        if (!entity.has_value()) {
            const auto data = duration_item->data(kTrackInfoRole);
            if (data.isValid()) {
                entity = entityForExactTrack(data.value<musicbrain::TrackInfo>().trackNo);
            }
        }
        if (entity.has_value() && !entity->is_cue_file && entity->isFilePath()) {
            has_writable_track = true;
            break;
        }
    }

    write_album_tags_button_->setEnabled(has_writable_track);
}

void MusicbrainzEditPage::setCoverPreview(QLabel* image_label,
    QLabel* info_label,
    const QPixmap& image,
    size_t image_file_size) {
    if (image_label == nullptr || info_label == nullptr) {
        return;
    }

    if (image.isNull()) {
        image_label->setPixmap(QPixmap());
        info_label->setText("-"_str);
        return;
    }

    image_label->setPixmap(image_util::resizeImage(image, image_label->size()));
    info_label->setText(tr("%1x%2 (%3)")
        .arg(image.width())
        .arg(image.height())
        .arg(formatBytes(static_cast<quint64>(image_file_size))));
}

void MusicbrainzEditPage::updateOriginalCoverArt(const std::optional<PlayListEntity>& entity) {
    if (!entity.has_value() || !entity->isFilePath()) {
        setCoverPreview(ui_->originalCoverArt, ui_->originalCoverArtSize, QPixmap(), 0);
        return;
    }

    try {
        QPixmap image;
        size_t image_file_size = 0;
        auto reader = MakeMetadataReader();
        reader->Open(entity->file_path.toStdWString());
        if (tag_util::readEmbeddedCover(*reader, image, image_file_size)) {
            setCoverPreview(ui_->originalCoverArt, ui_->originalCoverArtSize, image, image_file_size);
            return;
        }
    }
    catch (...) {
    }

    setCoverPreview(ui_->originalCoverArt, ui_->originalCoverArtSize, QPixmap(), 0);
}

void MusicbrainzEditPage::updateNewCoverArt(const QString& release_id) {
    setCoverPreview(ui_->newCoverArt,
        ui_->newCoverArtSize,
        coverArtForRelease(release_id),
        coverArtFileSizeForRelease(release_id));
}

QPixmap MusicbrainzEditPage::coverArtForRelease(const QString& release_id) const {
    if (release_id.isEmpty()) {
        return {};
    }

    const auto itr = cover_art_map_.find(release_id);
    if (itr == cover_art_map_.end()) {
        return {};
    }
    return itr.value();
}

size_t MusicbrainzEditPage::coverArtFileSizeForRelease(const QString& release_id) const {
    if (release_id.isEmpty()) {
        return 0;
    }

    const auto itr = cover_art_size_map_.find(release_id);
    if (itr == cover_art_size_map_.end()) {
        return 0;
    }
    return itr.value();
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

    is_writing_ = true;
    completed_writes_ = 0;
    total_writes_ = 1;
    if (fetch_progress_bar_ != nullptr) {
        fetch_progress_bar_->setRange(0, total_writes_);
        fetch_progress_bar_->setValue(completed_writes_);
        updateWriteProgressText(tr("Writing"));
    }
    updateWriteTagButtons();

    try {
        auto writer = MakeMetadataWriter();
        writer->Open(entity.file_path.toStdWString());
        writer->WriteTrack(static_cast<uint32_t>(track.trackNo));
        writer->WriteTitle(track.title.toStdWString());
        writer->WriteAlbum(selected_album_.toStdWString());
        writer->WriteArtist(artist.toStdWString());
        if (writer->CanWriteEmbeddedCover()) {
            tag_util::writeEmbeddedCover(*writer, coverArtForRelease(selected_release_id_));
        }
        completed_writes_ = 1;
        if (fetch_progress_bar_ != nullptr) {
            fetch_progress_bar_->setValue(completed_writes_);
            updateWriteProgressText(tr("Writing"));
        }
    }
    catch (...) {
        is_writing_ = false;
        updateWriteProgressText(tr("Failed"));
        updateWriteTagButtons();
        XMessageBox::showError(tr("Failure to write tag!"));
        return;
    }

    try {
        qDaoFacade.music_dao.updateMusicTitle(entity.music_id, track.title);
        qDaoFacade.album_dao.updateAlbum(entity.album_id, selected_album_);
        qDaoFacade.artist_dao.updateArtist(entity.artist_id, artist);
    }
    catch (...) {
        is_writing_ = false;
        updateWriteProgressText(tr("Failed"));
        updateWriteTagButtons();
        XMessageBox::showError(tr("Failure to update database!"));
        return;
    }

    entity.track = static_cast<uint32_t>(track.trackNo);
    entity.title = track.title;
    entity.album = selected_album_;
    entity.artist = artist;
    selected_entity_ = entity;
    setTagPreview(selected_entity_, selected_track_, selected_album_);
    updateOriginalCoverArt(selected_entity_);

    is_writing_ = false;
    updateWriteProgressText(tr("Completed"));
    updateWriteTagButtons();
    XMessageBox::showInformation(tr("Write tag successfully!"));
}

void MusicbrainzEditPage::writeSelectedAlbumTags() {
    auto index = ui_->albumRecordingView->currentIndex();
    if (!index.isValid()) {
        return;
    }

    auto* scope_item = writeScopeItemForIndex(album_track_model_, index);
    if (scope_item == nullptr) {
        return;
    }

    struct AlbumWriteItem {
        PlayListEntity entity;
        musicbrain::TrackInfo track;
        QString album;
        QString release_id;
    };

    QList<AlbumWriteItem> write_items;
    auto skipped_tracks = 0;
    QList<QStandardItem*> track_items;
    collectTrackDurationItems(scope_item, track_items);
    for (auto* duration_item : track_items) {
        const auto track_data = duration_item->data(kTrackInfoRole);
        const auto album_data = duration_item->data(kAlbumTitleRole);
        if (!track_data.isValid() || !album_data.isValid()) {
            continue;
        }

        const auto track = track_data.value<musicbrain::TrackInfo>();
        auto entity = matchedEntityForItem(duration_item);
        if (!entity.has_value()) {
            entity = entityForExactTrack(track.trackNo);
        }
        if (!entity.has_value() || entity->is_cue_file || !entity->isFilePath()) {
            ++skipped_tracks;
            continue;
        }

        write_items.append({ *entity, track, album_data.value<QString>(), duration_item->data(kReleaseIdRole).toString() });
    }

    if (write_items.isEmpty()) {
        XMessageBox::showWarning(tr("No writable tracks were found in the selected album."));
        return;
    }

    auto confirm_text = tr("Write tags to %1 tracks in this %2?\n\nThis will update metadata files on disk.")
        .arg(write_items.size())
        .arg(writeScopeText(scope_item));
    if (skipped_tracks > 0) {
        confirm_text.append("\n\n"_str);
        confirm_text.append(tr("%1 tracks will be skipped because they can not be written.").arg(skipped_tracks));
    }

    if (XMessageBox::showYesOrNo(confirm_text) != QDialogButtonBox::Yes) {
        return;
    }

    is_writing_ = true;
    completed_writes_ = 0;
    total_writes_ = write_items.size();
    if (fetch_progress_bar_ != nullptr) {
        fetch_progress_bar_->setRange(0, total_writes_ > 0 ? total_writes_ : 1);
        fetch_progress_bar_->setValue(completed_writes_);
        updateWriteProgressText(tr("Writing"));
    }
    updateWriteTagButtons();

    try {
        for (const auto& item : write_items) {
            const auto artist = artistText(item.track);
            auto writer = MakeMetadataWriter();
            writer->Open(item.entity.file_path.toStdWString());
            writer->WriteTrack(static_cast<uint32_t>(item.track.trackNo));
            writer->WriteTitle(item.track.title.toStdWString());
            writer->WriteAlbum(item.album.toStdWString());
            writer->WriteArtist(artist.toStdWString());
            if (writer->CanWriteEmbeddedCover()) {
                tag_util::writeEmbeddedCover(*writer, coverArtForRelease(item.release_id));
            }
            ++completed_writes_;
            if (fetch_progress_bar_ != nullptr) {
                fetch_progress_bar_->setValue(completed_writes_);
                updateWriteProgressText(tr("Writing"));
            }
        }
    }
    catch (...) {
        is_writing_ = false;
        updateWriteProgressText(tr("Failed"));
        updateWriteTagButtons();
        XMessageBox::showError(tr("Failure to write tag!"));
        return;
    }

    try {
        for (const auto& item : write_items) {
            const auto artist = artistText(item.track);
            qDaoFacade.music_dao.updateMusicTitle(item.entity.music_id, item.track.title);
            qDaoFacade.album_dao.updateAlbum(item.entity.album_id, item.album);
            qDaoFacade.artist_dao.updateArtist(item.entity.artist_id, artist);
        }
    }
    catch (...) {
        is_writing_ = false;
        updateWriteProgressText(tr("Failed"));
        updateWriteTagButtons();
        XMessageBox::showError(tr("Failure to update database!"));
        return;
    }

    for (const auto& item : write_items) {
        for (auto& entity : entities_) {
            if (entity.music_id != item.entity.music_id) {
                continue;
            }
            entity.track = static_cast<uint32_t>(item.track.trackNo);
            entity.title = item.track.title;
            entity.album = item.album;
            entity.artist = artistText(item.track);
            break;
        }
    }

    if (selected_entity_ && selected_track_) {
        selected_entity_->track = static_cast<uint32_t>(selected_track_->trackNo);
        selected_entity_->title = selected_track_->title;
        selected_entity_->album = selected_album_;
        selected_entity_->artist = artistText(*selected_track_);
        setTagPreview(selected_entity_, selected_track_, selected_album_);
        updateOriginalCoverArt(selected_entity_);
    }

    is_writing_ = false;
    updateWriteProgressText(tr("Completed"));
    updateWriteTagButtons();
    XMessageBox::showInformation(tr("Write album tags successfully!"));
}

void MusicbrainzEditPage::closeEvent(QCloseEvent* event) {
    if (is_fetching_ || is_writing_) {
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
        if (is_writing_) {
            updateWriteProgressText(tr("Writing"));
        }
        else {
            updateFetchProgressText(is_fetching_ ? tr("Fetching") : tr("Completed"));
        }
    }
    if (write_tag_button_ != nullptr) {
        write_tag_button_->setText(tr("Write Selected Track"));
        write_tag_button_->setToolTip(tr("Write tags only to the selected track."));
    }
    if (write_album_tags_button_ != nullptr) {
        write_album_tags_button_->setText(tr("Write Entire Album"));
        write_album_tags_button_->setToolTip(tr("Write tags to every matched track in the selected album."));
    }
}
