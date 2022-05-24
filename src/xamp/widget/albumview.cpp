#include <vector>

#include <QPainter>
#include <QScrollBar>
#include <QApplication>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSpacerItem>
#include <QHeaderView>
#include <QClipboard>
#include <QStandardPaths>
#include <QFileDialog>
#include <QSqlQuery>
#include <QTimer>

#include <stream/api.h>

#include <widget/appsettings.h>
#include <widget/database.h>
#include <widget/playlisttableview.h>
#include <widget/actionmap.h>
#include <widget/clickablelabel.h>

#if defined(Q_OS_WIN)
#include <widget/win32/win32.h>
#endif

#include "thememanager.h"
#include <widget/str_utilts.h>
#include <widget/image_utiltis.h>
#include <widget/pixmapcache.h>
#include <widget/albumview.h>

using namespace xamp::stream;

AlbumViewStyledDelegate::AlbumViewStyledDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
    , text_color_(Qt::black) {
}

void AlbumViewStyledDelegate::setTextColor(QColor color) {
    text_color_ = color;
}

void AlbumViewStyledDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (!index.isValid()) {
        return;
    }

    painter->setRenderHints(QPainter::Antialiasing, true);
    painter->setRenderHints(QPainter::SmoothPixmapTransform, true);

    auto album = index.model()->data(index.model()->index(index.row(), 0)).toString();
    auto cover_id = index.model()->data(index.model()->index(index.row(), 1)).toString();
    auto artist = index.model()->data(index.model()->index(index.row(), 2)).toString();

    const auto default_cover_size = qTheme.getDefaultCoverSize();
    const QRect cover_rect(option.rect.left() + 10, option.rect.top() + 10,
        default_cover_size.width(), default_cover_size.height());

    QRect album_text_rect(option.rect.left() + 10,
        option.rect.top() + default_cover_size.height() + 15,
        option.rect.width() - 30, 15);
    QRect artist_text_rect(option.rect.left() + 10,
        option.rect.top() + default_cover_size.height() + 35,
        option.rect.width() - 30, 15);

    painter->setPen(QPen(text_color_));

    auto f = painter->font();
#ifdef Q_OS_WIN
    f.setPointSize(8);
#endif
    f.setBold(true);
    painter->setFont(f);
    painter->drawText(album_text_rect, Qt::AlignVCenter, album);

    painter->setPen(QPen(Qt::gray));

    f.setBold(false);
    painter->setFont(f);
    painter->drawText(artist_text_rect, Qt::AlignVCenter, artist);

    auto album_cover = &qTheme.pixmap().defaultSizeUnknownCover();

    if (const auto * cache_small_cover = qPixmapCache.find(cover_id)) {
        album_cover = cache_small_cover;        
        painter->drawPixmap(cover_rect, Pixmap::roundImage(*album_cover, Pixmap::kSmallImageRadius));
    }
    else {
        painter->drawPixmap(cover_rect, Pixmap::roundImage(*album_cover, Pixmap::kSmallImageRadius));
    }
}

QSize AlbumViewStyledDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    auto result = QStyledItemDelegate::sizeHint(option, index);
    const auto default_cover = qTheme.getDefaultCoverSize();
    result.setWidth(default_cover.width() + 30);
    result.setHeight(default_cover.height() + 80);
    return result;
}

AlbumPlayListTableView::AlbumPlayListTableView(QWidget* parent)
    : QTableView(parent) {
    setModel(&model_);

    auto f = font();
#ifdef Q_OS_WIN
    f.setPointSize(9);
#else
    f.setPointSize(12);
#endif
    setFont(f);

    setUpdatesEnabled(true);
    setAcceptDrops(true);
    setDragEnabled(true);
    setShowGrid(false);

    setDragDropMode(InternalMove);
    setFrameShape(NoFrame);
    setFocusPolicy(Qt::NoFocus);

    setHorizontalScrollMode(ScrollPerPixel);
    setVerticalScrollMode(ScrollPerPixel);
    setSelectionMode(ExtendedSelection);
    setSelectionBehavior(SelectRows);

    verticalHeader()->setVisible(false);

    setColumnWidth(0, 5);
    horizontalHeader()->hide();
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    setStyleSheet(Q_UTF8("background-color: transparent"));
}

void AlbumPlayListTableView::resizeColumn() {
    auto header = horizontalHeader();

    for (auto column = 0; column < header->count(); ++column) {
        switch (column) {
        case AlbumPlayListTableView::PLAYLIST_TRACK:
            header->setSectionResizeMode(column, QHeaderView::ResizeToContents);
            header->resizeSection(column, 12);
            break;
        case AlbumPlayListTableView::PLAYLIST_TITLE:
            header->setSectionResizeMode(column, QHeaderView::Fixed);
            header->resizeSection(column, size().width() - qTheme.getAlbumCoverSize().width() - 100);
            break;
        case AlbumPlayListTableView::PLAYLIST_DURATION:
            header->setSectionResizeMode(column, QHeaderView::ResizeToContents);
            header->resizeSection(column, 60);
            break;
        default:
            header->setSectionResizeMode(column, QHeaderView::Stretch);
            break;
        }
    }
}

void AlbumPlayListTableView::setPlaylistMusic(int32_t album_id) {
    QString query = Q_UTF8(R"(
SELECT
    musics.track,
    musics.title,
    musics.durationStr,
    musics.musicId,
    artists.artist,
    musics.fileExt,
    musics.path,
    albums.coverId,
    albums.album,
    artists.artistId,
    albums.albumId
FROM
    albumMusic
    LEFT JOIN albums ON albums.albumId = albumMusic.albumId
    LEFT JOIN artists ON artists.artistId = albumMusic.artistId
    LEFT JOIN musics ON musics.musicId = albumMusic.musicId
WHERE
    albums.albumId = %1
ORDER BY musics.path
;)");
    model_.setQuery(query.arg(album_id));
    setColumnHidden(3, true);
    setColumnHidden(4, true);
    setColumnHidden(5, true);
    setColumnHidden(6, true);
    setColumnHidden(7, true);
    setColumnHidden(8, true);
    setColumnHidden(9, true);
    setColumnHidden(10, true);
    resizeColumn();
}

AlbumViewPage::AlbumViewPage(QWidget* parent)
    : QFrame(parent) {
    setFrameStyle(QFrame::NoFrame);

    auto default_layout = new QVBoxLayout(this);
    default_layout->setSpacing(0);
    default_layout->setContentsMargins(10, 10, 10, 0);

    auto close_button = new QPushButton(this);
    close_button->setObjectName(Q_UTF8("albumViewPageCloseButton"));
    close_button->setStyleSheet(Q_UTF8(R"(
                                         QPushButton#albumViewPageCloseButton {
                                         border: none;
                                         image: url(:/xamp/Resource/%1/close_normal.png);
                                         background-color: transparent;
                                         }
										 QPushButton#albumViewPageCloseButton:hover {	
										 image: url(:/xamp/Resource/%1/close_hover.png);									 
                                         }
                                         )").arg(qTheme.themeColorPath()));
    close_button->setFixedSize(QSize(24, 24));

    auto hbox_layout = new QHBoxLayout();
    hbox_layout->setSpacing(0);
    hbox_layout->setContentsMargins(0, 0, 0, 0);

    auto f = font();

    f.setPointSize(25);
    f.setBold(true);
    album_ = new QLabel(this);
    album_->setMaximumSize(QSize(16777215, 32));
    album_->setFont(f);
    album_->setStyleSheet(Q_UTF8("background-color: transparent;"));

    auto artist_layout = new QHBoxLayout();
    artist_layout->setSpacing(0);
    artist_layout->setContentsMargins(0, 0, 0, 0);

    f.setPointSize(14);
    f.setBold(false);
    artist_ = new ClickableLabel(this);
    artist_->setMaximumSize(QSize(400, 32));
    artist_->setFont(f);
    artist_->setAlignment(Qt::AlignLeft);
    artist_->setStyleSheet(Q_UTF8("background-color: transparent;"));

    tracks_ = new QLabel(this);
    tracks_->setFixedSize(QSize(16777215, 64));
    tracks_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    tracks_->setFont(f);
    tracks_->setStyleSheet(Q_UTF8("background-color: transparent;"));

    durtions_ = new QLabel(this);
    durtions_->setFixedSize(QSize(16777215, 64));
    durtions_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    durtions_->setFont(f);
    durtions_->setStyleSheet(Q_UTF8("background-color: transparent;"));

    artist_layout->addWidget(artist_);
    artist_layout->addWidget(tracks_);
    artist_layout->addWidget(durtions_);
    artist_layout->setStretch(0, 0);
    artist_layout->setStretch(1, 2);
    artist_layout->setStretch(2, 2);

    auto button_spacer = new QSpacerItem(20, 10, QSizePolicy::Expanding, QSizePolicy::Expanding);
    hbox_layout->addWidget(close_button);
    hbox_layout->addSpacerItem(button_spacer);

    default_layout->addLayout(hbox_layout);
    default_layout->addWidget(album_);
    default_layout->addLayout(artist_layout);

    cover_ = new QLabel(this);
    cover_->setMinimumSize(QSize(325, 325));
    cover_->setMaximumSize(QSize(325, 325));
    cover_->setStyleSheet(Q_UTF8("background-color: transparent;"));

    auto* playlist_layout = new QHBoxLayout();
    playlist_ = new AlbumPlayListTableView(this);
    playlist_layout->addWidget(playlist_);
    playlist_layout->addWidget(cover_);
    default_layout->setStretch(2, 1);

    default_layout->addLayout(playlist_layout);
    default_layout->setStretch(1, 0);
    default_layout->setStretch(3, 2);

    (void)QObject::connect(close_button, &QPushButton::clicked, [this]() {
        hide();
        });

    (void)QObject::connect(artist_, &ClickableLabel::clicked, [this]() {
        emit clickedArtist(artist_->text(), artist_cover_id_, artist_id_);
        });

    (void)QObject::connect(playlist_, &QTableView::doubleClicked, [this](const QModelIndex& index) {
        emit playMusic(getAlbumEntity(index));
        });

    qTheme.setBackgroundColor(this);
}

void AlbumViewPage::setAlbum(const QString& album) {
    album_->setText(album);
}

void AlbumViewPage::setArtistId(int32_t artist_id) {
    artist_id_ = artist_id;
}

void AlbumViewPage::setArtist(const QString& artist) {
    artist_->setText(artist);
}

void AlbumViewPage::setPlaylistMusic(int32_t album_id) {
    playlist_->setPlaylistMusic(album_id);
}

void AlbumViewPage::setArtistCover(const QString& cover_id) {
    artist_cover_id_ = cover_id;
}

void AlbumViewPage::setTracks(int32_t tracks) {
    tracks_->setText(tr("%1 Songs").arg(tracks));
}

void AlbumViewPage::setTotalDuration(double durations) {
    durtions_->setText(msToString(durations));
}

void AlbumViewPage::setCover(const QString& cover_id) {
    if (auto const * cache_small_cover = qPixmapCache.find(cover_id)) {
        auto image = Pixmap::roundImage(Pixmap::resizeImage(cache_small_cover->copy(),
            qTheme.getAlbumCoverSize()));
        cover_->setPixmap(image);
    }
    else {
        const auto image = Pixmap::roundImage(Pixmap::resizeImage(qTheme.pixmap().defaultSizeUnknownCover(),
                                                                  qTheme.getAlbumCoverSize()));
        cover_->setPixmap(image);
    }
}

AlbumView::AlbumView(QWidget* parent)
    : QListView(parent)
    , page_(new AlbumViewPage(this))
	, model_(this) {
    setModel(&model_);
    refreshOnece();
    setUniformItemSizes(true);
    setDragEnabled(false);
    setSelectionRectVisible(false);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setWrapping(true);
    setFlow(QListView::LeftToRight);
    setViewMode(QListView::IconMode);
    setResizeMode(QListView::Adjust);
    setFrameStyle(QFrame::NoFrame);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setItemDelegate(new AlbumViewStyledDelegate(this));
    setAutoScroll(false);
    viewport()->setAttribute(Qt::WA_StaticContents);

    page_->hide();

    (void)QObject::connect(page_,
                            &AlbumViewPage::playMusic,
                            this,
                            &AlbumView::playMusic);

    (void)QObject::connect(page_,
                            &AlbumViewPage::clickedArtist,
                            this,
                            &AlbumView::clickedArtist);

    (void)QObject::connect(this, &QListView::clicked, [this](auto index) {
        auto album = getIndexValue(index, 0).toString();
        auto cover_id = getIndexValue(index, 1).toString();
        auto artist = getIndexValue(index, 2).toString();
        auto album_id = getIndexValue(index, 3).toInt();
        auto artist_id = getIndexValue(index, 4).toInt();
        auto artist_cover_id = getIndexValue(index, 5).toString();

        const auto list_view_rect = this->rect();

        page_->setAlbum(album);
        page_->setArtist(artist);
        page_->setArtistId(artist_id);
        page_->setCover(cover_id);
        page_->setArtistCover(artist_cover_id);
        page_->setPlaylistMusic(album_id);
        page_->setFixedSize(QSize(list_view_rect.size().width() - 10, list_view_rect.height() - 6));

        if (auto album_stats = qDatabase.getAlbumStats(album_id)) {
            page_->setTracks(album_stats.value().tracks);
            page_->setTotalDuration(album_stats.value().durations);
        }

        page_->move(QPoint(list_view_rect.x() + 5, 3));
        qTheme.setBackgroundColor(page_);
    	
        page_->show();
        });

    (void)QObject::connect(verticalScrollBar(), &QScrollBar::valueChanged, [this](auto) {
        hideWidget();
        });

    setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(this, &QTableView::customContextMenuRequested, [this](const QPoint& pt) {
        auto index = indexAt(pt);

        ActionMap<AlbumView> action_map(this);

        auto removeAlbum = [=]() {
            for (const auto album_id : qDatabase.getAlbumId()) {
                qDatabase.removeAlbum(album_id);
            }
            qDatabase.removeAllArtist();
            update();
            emit removeAll();
            qPixmapCache.clear();
        };

        if (index.isValid()) {
            auto album = getIndexValue(index, 0).toString();
            auto artist = getIndexValue(index, 2).toString();
            auto album_id = getIndexValue(index, 3).toInt();
            auto artist_id = getIndexValue(index, 4).toInt();
            auto artist_cover_id = getIndexValue(index, 5).toString();

            action_map.addAction(tr("Add album to playlist"), [=]() {
                std::vector<PlayListEntity> entities;
                std::vector<int32_t> add_playlist_music_ids;
                qDatabase.forEachAlbumMusic(album_id,
                    [&entities, &add_playlist_music_ids](const PlayListEntity& entity) mutable {
                        if (entity.album_replay_gain == 0.0) {
                            entities.push_back(entity);
                        }
                        add_playlist_music_ids.push_back(entity.music_id);
                    });
                emit addPlaylist(add_playlist_music_ids, entities);
                });

            action_map.addAction(tr("Add album (artist) to playlist"), [=]() {
                std::vector<PlayListEntity> entities;
                std::vector<int32_t> add_playlist_music_ids;
                qDatabase.forEachAlbumArtistMusic(album_id, artist_id,
                    [&entities, &add_playlist_music_ids](const PlayListEntity& entity) mutable {
                        if (entity.album_replay_gain == 0.0) {
                            entities.push_back(entity);
                        }
                        add_playlist_music_ids.push_back(entity.music_id);
                    });
                emit addPlaylist(add_playlist_music_ids, entities);
                });

            action_map.addAction(tr("Copy album"), [album]() {
                QApplication::clipboard()->setText(album);
            });

            action_map.addAction(tr("Copy artist"), [artist]() {
                QApplication::clipboard()->setText(artist);
            });

            action_map.addSeparator();

            action_map.addAction(tr("Remove select album"), [=]() {
                qDatabase.removeAlbum(album_id);
                refreshOnece();
                });

            action_map.addAction(tr("Remove all album"), [=]() {
                removeAlbum();
                });
        } else {
            action_map.addSeparator();
            action_map.addAction(tr("Remove select album"));
            action_map.addAction(tr("Remove all album"), [=]() {
                removeAlbum();
            });
            action_map.addSeparator();
            action_map.addAction(tr("Add album to playlist"));
            action_map.addAction(tr("Copy album"));
            action_map.addAction(tr("Copy artist"));
        }

        action_map.addSeparator();

        (void)action_map.addAction(tr("Load local file"), [this]() {
            QString exts(Q_UTF8("("));
            for (const auto & file_ext : GetSupportFileExtensions()) {
                exts += Q_UTF8("*") + QString::fromStdString(file_ext);
                exts += Q_UTF8(" ");
            }
            exts += Q_UTF8(")");
            const auto file_name = QFileDialog::getOpenFileName(this,
                tr("Open file"),
                AppSettings::getMyMusicFolderPath(),
                tr("Music Files ") + exts,
                nullptr);
            if (file_name.isEmpty()) {
                return;
            }
            append(file_name);
            });

        (void)action_map.addAction(tr("Load file directory"), [this]() {
	        const auto dir_name = QFileDialog::getExistingDirectory(this,
	                                                                tr("Select a directory"),
	                                                                AppSettings::getMyMusicFolderPath(),
                QFileDialog::ShowDirsOnly);
            if (dir_name.isEmpty()) {
                return;
            }
            append(dir_name);
            });

        action_map.exec(pt);
    });

    setStyleSheet(Q_UTF8("background-color: transparent"));

    update();
}

void AlbumView::payNextMusic() {
    QModelIndex next_index;
    auto index = page_->playlist()->currentIndex();
    auto row_count = page_->playlist()->model()->rowCount();
    if (row_count == 0) {
        return;
    }
    if (index.row() + 1 >= row_count) {
        next_index = page_->playlist()->model()->index(0, 0);
    }
    else {
        next_index = page_->playlist()->model()->index(index.row() + 1, 0);
    }
    emit playMusic(getAlbumEntity(next_index));
    page_->playlist()->setCurrentIndex(next_index);
}

void AlbumView::onThemeChanged(QColor backgroundColor, QColor color) {
    dynamic_cast<AlbumViewStyledDelegate*>(itemDelegate())->setTextColor(color);
    page_->setStyleSheet(backgroundColorToString(backgroundColor));
}

void AlbumView::setFilterByArtistId(int32_t artist_id) {
    model_.setQuery(Q_STR(R"(
    SELECT
        album,
        albums.coverId,
        artist,
        albums.albumId,
        artists.artistId,
        artists.coverId
    FROM
        albumArtist
    LEFT JOIN
        albums ON albums.albumId = albumArtist.albumId
    LEFT JOIN
        artists ON artists.artistId = albumArtist.artistId
    WHERE
        (artists.artistId = '%1') AND (albums.isPodcast = 0)
    )").arg(artist_id));
}

void AlbumView::setFilterByArtistFirstChar(const QString &first_char) {
    QString s(
    Q_UTF8(R"(
SELECT
    album,
    albums.coverId,
    artist,
    albums.albumId,
    artists.artistId,
    artists.coverId
FROM
    albumArtist
LEFT JOIN
    albums ON albums.albumId = albumArtist.albumId
LEFT JOIN
    artists ON artists.artistId = albumArtist.artistId
WHERE
    (artists.firstChar = '%1') AND (albums.isPodcast = 0)
)")
    );

    model_.setQuery(s.arg(first_char));
}

void AlbumView::update() {
    model_.setQuery(Q_UTF8(R"(
SELECT
    albums.album,
    albums.coverId,
    artists.artist,
    albums.albumId,
    artists.artistId,
    artists.coverId
FROM
    albums
LEFT 
	JOIN artists ON artists.artistId = albums.artistId
WHERE 
	albums.isPodcast = 0
LIMIT 200
    )"));
}

void AlbumView::refreshOnece() {
    update();
}

void AlbumView::onSearchTextChanged(const QString& text) {
    QString query(Q_UTF8(R"(
SELECT
    albums.album,
    albums.coverId,
    artists.artist,
    albums.albumId,
    artists.artistId,
    artists.coverId
FROM
    albums
    LEFT JOIN artists ON artists.artistId = albums.artistId
WHERE
    (
    albums.album LIKE '%%1%' OR artists.artist LIKE '%%1%'
    ) AND (albums.isPodcast = 0)
LIMIT 200
    )"));
    model_.setQuery(query.arg(text));
}

void AlbumView::append(const QString& file_name) {
    auto adapter = QSharedPointer<MetadataExtractAdapter>(new MetadataExtractAdapter());

    (void) QObject::connect(adapter.get(),
                            &MetadataExtractAdapter::readCompleted,
                            this,
                            &AlbumView::processMeatadata);    
    MetadataExtractAdapter::readFileMetadata(adapter, file_name);
}

void AlbumView::processMeatadata(int64_t dir_last_write_time, const std::forward_list<Metadata> &medata) {
    MetadataExtractAdapter::processMetadata(dir_last_write_time, medata);
    emit loadCompleted();
}

void AlbumView::hideWidget() {
    if (!page_) {
        return;
    }
    page_->hide();
}