#include <QStandardItemModel>
#include <ui_musicbrainzeditpage.h>
#include <widget/musicbrainzeditpage.h>
#include <widget/util/image_util.h>
#include <widget/util/ui_util.h>

namespace {
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

 //   QList<FileMeta> metas;

 //   Q_FOREACH(auto entity, entities) {
 //       auto* child1 = new QStandardItem(QString::number(entity.track));
 //       auto* child2 = new QStandardItem(entity.title);
 //       auto* child3 = new QStandardItem(formatDuration(entity.duration));
 //       child3->setData(QVariant::fromValue(entity), Qt::UserRole + 1);
 //       QList<QStandardItem*> row_items;
 //       row_items << child1 << child2 << child3;
 //       album_item->appendRow(row_items);

 //       FileMeta file_meta;
 //       file_meta.album = entity.album;
 //       file_meta.albumArtist = entity.artist;
 //       file_meta.artist = entity.artist;
 //       file_meta.title = entity.title;
 //       file_meta.totalTracks = entities.count();
 //       file_meta.totalAlbumTracks = entities.count();
 //       file_meta.date = entity.getDateTime();
	//	file_meta.lengthMs = entity.duration * 1000;

 //       metas.push_back(file_meta);
 //   }

 //   ui_->trackView->expand(album_item->index());
	//ui_->trackView->setModel(track_model_);

 //   // Album tracks
 //   QStringList album_track_headers;
 //   album_track_headers << tr("Track") << tr("Title") << tr("Duration") << tr("Similarity");
 //   album_track_model_->setColumnCount(album_track_headers.size());
 //   album_track_model_->setHorizontalHeaderLabels(album_track_headers);
 //   ui_->albumRecordingView->setModel(album_track_model_);
 //   ui_->albumRecordingView->setRootIsDecorated(true);
 //   ui_->albumRecordingView->setAllColumnsShowFocus(true);

 //   Weights w;
 //   Preferences pref;
 //   std::vector<std::vector<MatchTrackResult>> match_albums;

 //   for (const auto &albums : recording_list) {
 //       std::vector<MatchTrackResult> match_track_results;
 //       for (const auto& recording : albums.recordings) {
 //           top_row.clear();
 //           if (recording.tracks.front().lengthMs == -1) {
 //               continue;
 //           }
 //           auto* item = new QStandardItem(recording.title);
 //           top_row << item
 //               << new QStandardItem(QString())
 //               << new QStandardItem(QString())
 //               << new QStandardItem(QString());
 //           if (recording.tracks.count() != entities.count()) {
 //               continue;
 //           }
 //           int i = 0;
 //           for (const auto& track : recording.tracks) {
 //               auto* child1 = new QStandardItem(QString::number(track.trackNo));
 //               auto* child2 = new QStandardItem(track.title);
 //               auto* child3 = new QStandardItem(formatDuration(track.lengthMs / 1000.0));
 //               auto* child4 = new QStandardItem();
 //               child3->setData(recording.release_id, Qt::UserRole + 1);
 //               child3->setData(QVariant::fromValue(track), Qt::UserRole + 2);
 //               child3->setData(recording.title, Qt::UserRole + 3);
 //               QList<QStandardItem*> row_items;
 //               row_items << child1 << child2 << child3 << child4;
 //               item->appendRow(row_items);
	//			if (i < metas.size()) {
 //                   if (auto m = compareToTrack(metas[i], track, w, pref)) {
 //                       match_track_results.push_back(m.value());
 //                       child4->setText(QString::number(m.value().similarity));
 //                   }
	//			}
 //               ++i;
 //           }
 //           match_albums.push_back(match_track_results);
 //           cover_art_map_.insert(recording.release_id, recording.cover_art);
 //           album_track_model_->appendRow(top_row);
 //           ui_->albumRecordingView->expand(item->index());
 //       }
 //   }

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
