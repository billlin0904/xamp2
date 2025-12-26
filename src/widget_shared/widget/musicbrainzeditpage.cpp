#include <QStandardItemModel>
#include <ui_musicbrainzeditpage.h>
#include <widget/musicbrainzeditpage.h>
#include <widget/util/image_util.h>
#include <widget/util/ui_util.h>

namespace {
    constexpr int LENGTH_SCORE_THRES_MS = 30000;

    struct Weights {
        // Track-level
        double title = 22;
        double artist = 6;
        double length = 8;
        double isVideo = 4;
        // Release-level
        double album = 12;
        double albumArtist = 6;
        double totalTracks = 5;
        double totalAlbumTracks = 4;
        double date = 6;
        double releaseCountry = 3;
        double format = 2.5;
        double releaseType = 5;
        double rgLoadedBonus = 6; // if same release-group already loaded
    };

    struct Part {
        explicit Part(double score = 0, double weight = 0)
	        : score(score)
			, weight(weight) {
        }
	    double score;
    	double weight;
    };

    struct Preferences {
        // Higher priority comes first
        QStringList preferredCountries; // e.g. {"JP", "US", "GB"}
        QStringList preferredFormats; // e.g. {"CD", "Digital Media", "Vinyl"}
        QHash<QString, double> releaseTypeScores; // e.g. {"Album":1.0,"Live":0.2,"Other":0.5}
    };

    struct FileMeta {
        QString title;
        QString artist;
        QString album;
        QString albumArtist;
        int lengthMs = 0; // file duration
        bool isVideo = false;
        int totalTracks = 0; // disc track-count in your tags
        int totalAlbumTracks = 0; // entire release track-count if known; fallback to totalTracks
        QString date; // ISO-ish: YYYY or YYYY-MM or YYYY-MM-DD
    };

    struct MatchTrackResult {
        double similarity;
        musicbrain::Release bestRelease;
        musicbrain::TrackInfo track;
    };

    double linearWeightedAverage(const QVector<Part>& parts) {
        double num = 0.0, den = 0.0;
        for (const auto& p : parts) { num += p.score * p.weight; den += p.weight; }
        return den > 0.0 ? (num / den) : 0.0;
    }

    double countryPreferenceScore(const QString& releaseCountry, const QStringList& prefs) {
        if (prefs.isEmpty())
            return 0.0;
        int idx = prefs.indexOf(releaseCountry);
        if (idx < 0) 
            return 0.0;
        const double N = static_cast<double>(prefs.size());
        return (N - idx) / N; // front gets 1.0, end ~ 1/N
    }

    double lengthScore(int aMs, int bMs) {
        if (aMs <= 0 || bMs <= 0)
            return 0.0;
        const int diff = qAbs(aMs - bMs);
        const int clamped = std::min(diff, LENGTH_SCORE_THRES_MS);
        return 1.0 - static_cast<double>(clamped) / static_cast<double>(LENGTH_SCORE_THRES_MS);
    }

    double trackcountScore(int actual, int expected) {
        if (expected <= 0 || actual <= 0) return 0.0;
        if (actual == expected) return 1.0;
        if (actual < expected) return 0.3; // could be partial tagging
        return 0.0; // more than release track-count is suspicious
    }

    int extractYear(const QString& iso) {
        // Accept YYYY or YYYY-MM[-DD]
        if (iso.size() >= 4) {
            bool ok = false;
        	int y = iso.left(4).toInt(&ok);
        	if (ok) 
                return y;
        }
        return std::numeric_limits<int>::min();
    }

    double formatsPreferenceScore(const QStringList& formats, const QStringList& prefs) {
        if (prefs.isEmpty() || formats.isEmpty()) 
            return 0.0;
        double sum = 0.0;
    	int cnt = 0;
        for (const auto& f : formats) {
            int idx = prefs.indexOf(f);
            if (idx >= 0) {
	            sum += (static_cast<double>(prefs.size()) - idx) / static_cast<double>(prefs.size());
            }
            ++cnt;
        }
        return cnt > 0 ? sum / static_cast<double>(cnt) : 0.0;
    }

    double dateMatchFactor(const QString& metaDate, const QString& releaseDate) {
        // Mirrors Picard's logic
        // factors
        const double exact = 1.00;
        const double year = 0.95;
        const double closeYear = 0.85;
        const double existsVsNull = 0.65;
        const double noReleaseDate = 0.25;
    	const double differed = 0.0;
        if (!releaseDate.isEmpty()) {
            if (!metaDate.isEmpty()) {
                if (releaseDate == metaDate) return exact;
                const int ry = extractYear(releaseDate);
                const int my = extractYear(metaDate);
                if (ry != std::numeric_limits<int>::min() && my != std::numeric_limits<int>::min()) {
                    if (ry == my) return year;
                    if (qAbs(ry - my) <= 2) return closeYear;
                    return differed;
                }
                return differed;
            }
            else {
                return existsVsNull;
            }
        }
        else {
            return noReleaseDate;
        }
    }

    double releaseTypeScore(const musicbrain::ReleaseGroup& r, const QHash<QString, double>& typeScores) {
        const double other = typeScores.value("Other"_str, 0.5);
        QList<QString> types;
        if (!r.primaryType.isEmpty()) {
            types << r.primaryType;
        	types += r.secondaryTypes;
        }
        if (types.isEmpty()) 
            return other;
        double sum = 0.0;
    	bool hasZero = false;
        for (const auto& t : types) {
            const double s = typeScores.value(t, other);
            if (qFuzzyIsNull(s))
                hasZero = true;
            sum += s;
        }
        const double avg = sum / static_cast<double>(types.size());
        // emulate Picard's skip behavior by returning a tiny score (will be down-weighted heavily by caller)
        return hasZero ? 0.0 : avg;
    }

    double totalReleaseWeight(const Weights& w) {
        return w.album
    	+ w.albumArtist
    	+ w.totalTracks
    	+ w.totalAlbumTracks
    	+ w.date
    	+ w.releaseCountry
    	+ w.format
    	+ w.releaseType;
    }

    QVector<Part> compareToReleaseParts(const FileMeta& meta, const musicbrain::Release& r, const Weights& w, const Preferences& pref) {
        QVector<Part> parts;
        if (!meta.album.isEmpty()) 
            parts.emplace_back(similarity2(meta.album, r.title), w.album);

        if (meta.albumArtist.isEmpty()) {
            // In a full pipeline you'd extract MB release artist-credit; here we fall back to albumArtist vs title when missing
            parts.emplace_back(similarity2(meta.albumArtist, r.title), w.albumArtist * 0.5);
        }

    	/*if (meta.totalTracks > 0)
            parts.emplace_back(trackcountScore(meta.totalTracks, r.trackCount), w.totalTracks);
        {
            const int tat = meta.totalAlbumTracks > 0 ? meta.totalAlbumTracks : meta.totalTracks;
            if (tat > 0) 
                parts.emplace_back(trackcountScore(tat, r.trackCount), w.totalAlbumTracks);
        }

        parts.emplace_back(dateMatchFactor(meta.date, r.date), w.date);*/

        // preferences
        if (!pref.preferredCountries.isEmpty()) 
            parts.emplace_back(countryPreferenceScore(r.country, pref.preferredCountries), w.releaseCountry);

        if (!pref.preferredFormats.isEmpty()) 
            parts.emplace_back(formatsPreferenceScore(r.mediaFormats, pref.preferredFormats), w.format);

        if (!pref.releaseTypeScores.isEmpty()) 
            parts.emplace_back(releaseTypeScore(r.releaseGroup, pref.releaseTypeScores), w.releaseType);

        if (r.loadedInGroup) 
            parts.emplace_back(1.0, w.rgLoadedBonus);
        return parts;
    }

    std::optional<MatchTrackResult> compareToTrack(const FileMeta& meta, const musicbrain::TrackInfo& tc, const Weights& w, const Preferences& pref = Preferences()) {
        QVector<Part> base;
        if (!meta.title.isEmpty()) 
            base.emplace_back(similarity2(meta.title, tc.title), w.title);
        {
            const QString trackArtist = tc.artistCredits.isEmpty() ? QString() : tc.artistCredits.join(","_str);
            if (!meta.artist.isEmpty() && !trackArtist.isEmpty()) 
                base.emplace_back(similarity2(meta.artist, trackArtist), w.artist);
        }

    	base.emplace_back(lengthScore(meta.lengthMs, tc.lengthMs), w.length);
        base.emplace_back((meta.isVideo == tc.video) ? 1.0 : 0.0, w.isVideo);

        //const double searchScore = std::clamp(tc.mbSearchScore, 0.0, 1.0);
        const double searchScore = 1.0;

        // If no releases are attached, synthesize a fallback score using "Other" type baseline
   //     if (tc.releases.isEmpty()) {
   //         QVector<Part> temp = base;
   //         // Use neutral-ish preference hint
   //         const double other = pref.releaseTypeScores.value("Other"_str, 0.5);
   //         temp.emplace_back(other, totalReleaseWeight(w));
   //         const double sim = linearWeightedAverage(temp) * searchScore;
   //         MatchTrackResult best;
			//best.similarity = sim;
			//best.track = tc;
   //         return best;
   //     }
        if (tc.releases.isEmpty()) {
            return std::nullopt;
        }

        MatchTrackResult best;
    	best.similarity = -1.0;
    	best.track = tc;
        for (const auto& r : tc.releases) {
            QVector<Part> parts = base;
            const auto rparts = compareToReleaseParts(meta, r, w, pref);
            parts += rparts; // concatenate
            const double sim = linearWeightedAverage(parts) * searchScore;
            if (sim > best.similarity) {
	            best.similarity = sim;
            	best.bestRelease = r;
            }
        }
        return best;
    }
}

MusicbrainzEditPage::MusicbrainzEditPage(const QList<PlayListEntity>& entities, const QList<MusicBrainzAlbum>& recording_list, QWidget* parent)
    : QFrame(parent) {
    ui_ = new Ui::MusicbrainzEditPage();
    track_model_ = new QStandardItemModel(this);
    album_track_model_ = new QStandardItemModel(this);
	tag_model_ = new QStandardItemModel(this);
    ui_->setupUi(this);
	load(entities, recording_list);
}

MusicbrainzEditPage::~MusicbrainzEditPage() {
    delete ui_;
}

void MusicbrainzEditPage::load(const QList<PlayListEntity>& entities, const QList<MusicBrainzAlbum>& recording_list) {
    auto f = qTheme.defaultFont();
    f.setPointSize(qTheme.fontSize(9));
    setFont(f);
    ui_->trackView->setFont(f);
    ui_->albumRecordingView->setFont(f);

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

    QList<FileMeta> metas;

    Q_FOREACH(auto entity, entities) {
        auto* child1 = new QStandardItem(QString::number(entity.track));
        auto* child2 = new QStandardItem(entity.title);
        auto* child3 = new QStandardItem(formatDuration(entity.duration));
        child3->setData(QVariant::fromValue(entity), Qt::UserRole + 1);
        QList<QStandardItem*> row_items;
        row_items << child1 << child2 << child3;
        album_item->appendRow(row_items);

        FileMeta file_meta;
        file_meta.album = entity.album;
        file_meta.albumArtist = entity.artist;
        file_meta.artist = entity.artist;
        file_meta.title = entity.title;
        file_meta.totalTracks = entities.count();
        file_meta.totalAlbumTracks = entities.count();
        file_meta.date = entity.getDateTime();
		file_meta.lengthMs = entity.duration * 1000;

        metas.push_back(file_meta);
    }

    ui_->trackView->expand(album_item->index());
	ui_->trackView->setModel(track_model_);

    // Album tracks
    QStringList album_track_headers;
    album_track_headers << tr("Track") << tr("Title") << tr("Duration") << tr("Similarity");
    album_track_model_->setColumnCount(album_track_headers.size());
    album_track_model_->setHorizontalHeaderLabels(album_track_headers);
    ui_->albumRecordingView->setModel(album_track_model_);
    ui_->albumRecordingView->setRootIsDecorated(true);
    ui_->albumRecordingView->setAllColumnsShowFocus(true);

    Weights w;
    Preferences pref;
    std::vector<std::vector<MatchTrackResult>> match_albums;

    for (const auto &albums : recording_list) {
        std::vector<MatchTrackResult> match_track_results;
        for (const auto& recording : albums.recordings) {
            top_row.clear();
            if (recording.tracks.front().lengthMs == -1) {
                continue;
            }
            auto* item = new QStandardItem(recording.title);
            top_row << item
                << new QStandardItem(QString())
                << new QStandardItem(QString())
                << new QStandardItem(QString());
            if (recording.tracks.count() != entities.count()) {
                continue;
            }
            int i = 0;
            for (const auto& track : recording.tracks) {
                auto* child1 = new QStandardItem(QString::number(track.trackNo));
                auto* child2 = new QStandardItem(track.title);
                auto* child3 = new QStandardItem(formatDuration(track.lengthMs / 1000.0));
                auto* child4 = new QStandardItem();
                child3->setData(recording.release_id, Qt::UserRole + 1);
                child3->setData(QVariant::fromValue(track), Qt::UserRole + 2);
                child3->setData(recording.title, Qt::UserRole + 3);
                QList<QStandardItem*> row_items;
                row_items << child1 << child2 << child3 << child4;
                item->appendRow(row_items);
				if (i < metas.size()) {
                    if (auto m = compareToTrack(metas[i], track, w, pref)) {
                        match_track_results.push_back(m.value());
                        child4->setText(QString::number(m.value().similarity));
                    }
				}
                ++i;
            }
            match_albums.push_back(match_track_results);
            cover_art_map_.insert(recording.release_id, recording.cover_art);
            album_track_model_->appendRow(top_row);
            ui_->albumRecordingView->expand(item->index());
        }
    }

    (void)QObject::connect(ui_->albumRecordingView, &QTreeView::clicked,
        [this](const auto& index) {
        if (!index.isValid())
            return;
        auto idx = index.siblingAtColumn(2);
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

    tag_model_->setColumnCount(3);
    tag_model_->setHeaderData(0, Qt::Horizontal, tr("Tag"));
    tag_model_->setHeaderData(1, Qt::Horizontal, tr("Old value"));
    tag_model_->setHeaderData(2, Qt::Horizontal, tr("New value"));

	ui_->tagTableView->setModel(tag_model_);
    ui_->tagTableView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
    ui_->tagTableView->setFocusPolicy(Qt::StrongFocus);
    auto* header = ui_->tagTableView->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    setTabViewStyle(ui_->tagTableView);

    (void)QObject::connect(ui_->albumRecordingView, &QTreeView::clicked,
        [this](const auto& index) {
        auto idx = index.siblingAtColumn(2);
        if (!index.isValid())
           return;
        auto data = idx.data(Qt::UserRole + 2);
        if (!data.isValid())
           return;
        auto tracks = data.template value<musicbrain::TrackInfo>();
        data = idx.data(Qt::UserRole + 3);
        if (!data.isValid())
            return;
        auto album = data.template value<QString>();
        auto artist = tracks.artistCredits.join(","_str);
        tag_model_->setItem(0, 2, new QStandardItem(QString::number(tracks.trackNo)));
        tag_model_->setItem(1, 2, new QStandardItem(tracks.title));
        tag_model_->setItem(2, 2, new QStandardItem(album));
        tag_model_->setItem(3, 2, new QStandardItem(artist));
        tag_model_->setItem(4, 2, new QStandardItem(formatDuration(tracks.lengthMs / 1000.0)));
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
        tag_model_->setItem(0, 0, new QStandardItem("Track"_str));
        tag_model_->setItem(0, 1, new QStandardItem(QString::number(entity.track)));
        tag_model_->setItem(1, 0, new QStandardItem("Title"_str));
        tag_model_->setItem(1, 1, new QStandardItem(entity.title));
        tag_model_->setItem(2, 0, new QStandardItem("Album"_str));
        tag_model_->setItem(2, 1, new QStandardItem(entity.album));
        tag_model_->setItem(3, 0, new QStandardItem("Artist"_str));
        tag_model_->setItem(3, 1, new QStandardItem(entity.artist));
        tag_model_->setItem(4, 0, new QStandardItem("Duration"_str));
        tag_model_->setItem(4, 1, new QStandardItem(formatDuration(entity.duration)));
        });

}

void MusicbrainzEditPage::onThemeChangedFinished(ThemeColor theme_color) {    
}

void MusicbrainzEditPage::onRetranslateUi() {
    ui_->retranslateUi(this);
}
