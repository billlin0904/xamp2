#include <vector>

#include <QPainter>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QClipboard>
#include <QStandardPaths>
#include <QFileDialog>
#include <QSqlQuery>
#include <QPainterPath>

#include <widget/widget_shared.h>
#include <widget/scrolllabel.h>
#include <widget/appsettings.h>
#include <widget/database.h>
#include <widget/playlisttableview.h>
#include <widget/actionmap.h>
#include <widget/playlistpage.h>

#include "thememanager.h"
#include <widget/toast.h>
#include <widget/processindicator.h>
#include <widget/str_utilts.h>
#include <widget/image_utiltis.h>
#include <widget/pixmapcache.h>
#include <widget/ui_utilts.h>
#include <widget/albumview.h>

enum {
    INDEX_ALBUM = 0,
    INDEX_COVER,
    INDEX_ARTIST,
    INDEX_ALBUM_ID,
    INDEX_ARTIST_ID,
    INDEX_ARTIST_COVER_ID
};

AlbumViewStyledDelegate::AlbumViewStyledDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
    , text_color_(Qt::black)
	, more_album_opt_button_(new QPushButton())
	, play_button_(new QPushButton()) {
    more_album_opt_button_->setStyleSheet(Q_TEXT("background-color: transparent"));
    play_button_->setStyleSheet(Q_TEXT("background-color: transparent"));
    mask_image_ = Pixmap::roundDarkImage(qTheme.albumCoverSize());
}

void AlbumViewStyledDelegate::setTextColor(QColor color) {
    text_color_ = color;
}

void AlbumViewStyledDelegate::clearImageCache() {
    image_cache_.Clear();
}

bool AlbumViewStyledDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) {
    auto* ev = static_cast<QMouseEvent*> (event);
    mouse_point_ = ev->pos();
    auto current_cursor = QApplication::overrideCursor();

    const auto default_cover_size = qTheme.defaultCoverSize();
    constexpr auto icon_size = 24;
    const QRect more_button_rect(
        option.rect.left() + default_cover_size.width() - 10,
        option.rect.top() + default_cover_size.height() + 28,
        icon_size, icon_size);

    switch (ev->type()) {
    case QEvent::MouseButtonPress:
        if (ev->button() == Qt::LeftButton) {
            if (current_cursor != nullptr) {
                if (current_cursor->shape() == Qt::PointingHandCursor) {
                    emit enterAlbumView(index);
                }
            }
            if (more_button_rect.contains(mouse_point_)) {
                emit showAlbumOpertationMenu(index, mouse_point_);
            }
        }        
        break;
    default:
        break;
    }
    return true;
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

    const auto default_cover_size = qTheme.defaultCoverSize();
    const QRect cover_rect(option.rect.left() + 10,
        option.rect.top() + 10,
        default_cover_size.width(), 
        default_cover_size.height());

    QRect album_text_rect(option.rect.left() + 10,
        option.rect.top() + default_cover_size.height() + 15,
        option.rect.width() - 55,
        15);
    QRect artist_text_rect(option.rect.left() + 10,
        option.rect.top() + default_cover_size.height() + 35,
        option.rect.width() - 50, 
        15);

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

    auto* album_cover = &qTheme.defaultSizeUnknownCover();

    if (const auto* round_image = image_cache_.Find(cover_id)) {
        painter->drawPixmap(cover_rect, *round_image);
    } else {
        if (const auto* cache_small_cover = qPixmapCache.find(cover_id)) {
            album_cover = cache_small_cover;
        }
        auto album_image = Pixmap::roundImage(*album_cover, Pixmap::kSmallImageRadius);
        if (!cover_id.isEmpty()) {
            image_cache_.Add(cover_id, album_image);
        }
        painter->drawPixmap(cover_rect, album_image);
    }

    if (option.state & QStyle::State_MouseOver && cover_rect.contains(mouse_point_)) {
        painter->drawPixmap(cover_rect, mask_image_);

        constexpr auto icon_size = 48;
        constexpr auto offset = (icon_size / 2) - 10;

        const QRect button_rect(
            option.rect.left() + default_cover_size.width() / 2 - offset,
            option.rect.top() + default_cover_size.height() / 2 - offset,
            icon_size, icon_size);

        QStyleOptionButton button;
        button.rect = button_rect;
        button.icon = qTheme.playCircleIcon();
        button.state |= QStyle::State_Enabled;
        button.iconSize = QSize(icon_size, icon_size);
        QApplication::style()->drawControl(QStyle::CE_PushButton, &button, painter, play_button_.get());

        if (button_rect.contains(mouse_point_)) {
            QApplication::setOverrideCursor(Qt::PointingHandCursor);
            return;
        }
    }

    constexpr auto icon_size = 18;
    const QRect more_button_rect(
        option.rect.left() + default_cover_size.width() - 10,
        option.rect.top() + default_cover_size.height() + 30,
        icon_size, icon_size);
    
    QStyleOptionButton button;
    button.initFrom(more_album_opt_button_.get());
    button.rect = more_button_rect;
    button.icon = qTheme.moreIcon();
    button.state |= QStyle::State_Enabled;
    if (more_button_rect.contains(mouse_point_)) {
        button.state |= QStyle::State_Sunken;
        painter->setPen(qTheme.hoverColor());
        painter->setBrush(QBrush(qTheme.hoverColor()));
        painter->drawEllipse(more_button_rect);
    }

    if (more_album_opt_button_->isDefault()) {
        button.features = QStyleOptionButton::DefaultButton;
    }
    button.iconSize = QSize(icon_size, icon_size);
    QApplication::style()->drawControl(QStyle::CE_PushButton, &button, painter, more_album_opt_button_.get());

    QApplication::restoreOverrideCursor();
}

QSize AlbumViewStyledDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    auto result = QStyledItemDelegate::sizeHint(option, index);
    const auto default_cover = qTheme.defaultCoverSize();
    result.setWidth(default_cover.width() + 30);
    result.setHeight(default_cover.height() + 80);
    return result;
}

AlbumViewPage::AlbumViewPage(QWidget* parent)
    : QFrame(parent) {
    setFrameStyle(QFrame::NoFrame);

    auto* default_layout = new QVBoxLayout(this);
    default_layout->setSpacing(0);
    default_layout->setContentsMargins(0, 5, 0, 0);

    auto* close_button = new QPushButton(this);
    close_button->setObjectName(Q_TEXT("albumViewPageCloseButton"));
    close_button->setStyleSheet(Q_TEXT(R"(
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

    auto* hbox_layout = new QHBoxLayout();
    hbox_layout->setSpacing(0);
    hbox_layout->setContentsMargins(0, 0, 0, 0);

    auto* button_spacer = new QSpacerItem(20, 10, QSizePolicy::Expanding, QSizePolicy::Expanding);
    hbox_layout->addWidget(close_button);
    hbox_layout->addSpacerItem(button_spacer);

    page_ = new PlaylistPage(this);
    page_->playlist()->setPlaylistId(kDefaultAlbumPlaylistId);

    default_layout->addLayout(hbox_layout);
    default_layout->addWidget(page_);
    default_layout->setStretch(0, 0);
    default_layout->setStretch(1, 1);

    (void)QObject::connect(close_button, &QPushButton::clicked, [this]() {
        hide();
        });
    qTheme.setBackgroundColor(this);
}

void AlbumViewPage::setPlaylistMusic(const QString& album, int32_t album_id, const QString &cover_id) {
    page_->show();

    page_->playlist()->removeAll();
    Vector<int32_t> add_playlist_music_ids;

    add_playlist_music_ids.reserve(20);

    qDatabase.forEachAlbumMusic(album_id,
        [&add_playlist_music_ids](const PlayListEntity& entity) mutable {
            add_playlist_music_ids.push_back(entity.music_id);
        });

    qDatabase.addMusicToPlaylist(add_playlist_music_ids, page_->playlist()->playlistId());

    page_->playlist()->updateData();
    page_->title()->setText(album);
    page_->setCover(cover_id);

    if (const auto album_stats = qDatabase.getAlbumStats(album_id)) {
        page_->format()->setText(tr("%1 Tracks, %2, %3")
            .arg(QString::number(album_stats.value().tracks))
            .arg(msToString(album_stats.value().durations))
            .arg(QString::number(album_stats.value().year)));
    }
}

AlbumView::AlbumView(QWidget* parent)
    : QListView(parent)
    , page_(new AlbumViewPage(this))
	, styled_delegate_(new AlbumViewStyledDelegate(this))
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
    setItemDelegate(styled_delegate_);
    setAutoScroll(false);
    viewport()->setAttribute(Qt::WA_StaticContents);
    setMouseTracking(true);

    qTheme.setBackgroundColor(page_);
    page_->hide();

    (void)QObject::connect(page_,
                            &AlbumViewPage::clickedArtist,
                            this,
                            &AlbumView::clickedArtist);

    (void)QObject::connect(styled_delegate_, &AlbumViewStyledDelegate::showAlbumOpertationMenu, [this](auto index, auto pt) {
        showOperationMenu(pt);
        });

    (void)QObject::connect(styled_delegate_, &AlbumViewStyledDelegate::enterAlbumView, [this](auto index) {
        auto album = getIndexValue(index, INDEX_ALBUM).toString();
        auto cover_id = getIndexValue(index, INDEX_COVER).toString();
        auto artist = getIndexValue(index, INDEX_ARTIST).toString();
        auto album_id = getIndexValue(index, INDEX_ALBUM_ID).toInt();
        auto artist_id = getIndexValue(index, INDEX_ARTIST_ID).toInt();
        auto artist_cover_id = getIndexValue(index, INDEX_ARTIST_COVER_ID).toString();

        const auto list_view_rect = this->rect();
        page_->setPlaylistMusic(album, album_id, cover_id);
        page_->setFixedSize(QSize(list_view_rect.size().width() - 15, list_view_rect.height() - 6));
        page_->move(QPoint(list_view_rect.x() + 5, 3));

        if (enable_page_) {
            page_->show();
        }
        emit clickedAlbum(album, album_id, cover_id);
        });

    (void)QObject::connect(verticalScrollBar(), &QScrollBar::valueChanged, [this](auto) {
        hideWidget();
        });

    setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(this, &QTableView::customContextMenuRequested, [this](auto pt) {
        showAlbumViewMenu(pt);
    });

    setStyleSheet(Q_TEXT("background-color: transparent"));

    update();
}

void AlbumView::showAlbumViewMenu(const QPoint& pt) {
    auto index = indexAt(pt);

    ActionMap<AlbumView> action_map(this);

    auto removeAlbum = [=]() {
        auto* indicator = new ProcessIndicator(this);
        indicator->startAnimation();
        try {
            qDatabase.forEachAlbum([](auto album_id) {
                qDatabase.removeAlbum(album_id);
                });
            qDatabase.removeAllArtist();
            update();
            emit removeAll();
            qPixmapCache.clear();
        }
        catch (...) {
        }
        indicator->deleteLater();
    };

    auto* remove_all_album_act = action_map.addAction(tr("Remove all album"), [=]() {
        removeAlbum();
        styled_delegate_->clearImageCache();
        });
    remove_all_album_act->setIcon(qTheme.iconFromFont(0xe92b));

    auto* load_file_act = action_map.addAction(tr("Load local file"), [this]() {
        const auto file_name = QFileDialog::getOpenFileName(this,
            tr("Open file"),
            AppSettings::getMyMusicFolderPath(),
            tr("Music Files ") + getFileExtensions(),
            nullptr);
        if (file_name.isEmpty()) {
            return;
        }
        append(file_name);
        });
    load_file_act->setIcon(qTheme.iconFromFont(0xe89c));

    auto* load_dir_act = action_map.addAction(tr("Load file directory"), [this]() {
        const auto dir_name = QFileDialog::getExistingDirectory(this,
            tr("Select a directory"),
            AppSettings::getMyMusicFolderPath(),
            QFileDialog::ShowDirsOnly);
        if (dir_name.isEmpty()) {
            return;
        }
        append(dir_name);
        });
    load_dir_act->setIcon(qTheme.iconFromFont(0xe2cc));

    action_map.exec(pt);
}

void AlbumView::showOperationMenu(const QPoint &pt) {
    auto index = indexAt(pt);

    ActionMap<AlbumView> action_map(this);

    auto album = getIndexValue(index, INDEX_ALBUM).toString();
    auto artist = getIndexValue(index, INDEX_ARTIST).toString();
    auto album_id = getIndexValue(index, INDEX_ALBUM_ID).toInt();
    auto artist_id = getIndexValue(index, INDEX_ARTIST_ID).toInt();
    auto artist_cover_id = getIndexValue(index, INDEX_ARTIST_COVER_ID).toString();

    auto* add_album_to_playlist_act = action_map.addAction(tr("Add album to playlist"), [=]() {
        Vector<PlayListEntity> entities;
        Vector<int32_t> add_playlist_music_ids;
        qDatabase.forEachAlbumArtistMusic(album_id, artist_id,
            [&entities, &add_playlist_music_ids](const PlayListEntity& entity) mutable {
                if (entity.album_replay_gain == 0.0) {
                    entities.push_back(entity);
                }
                add_playlist_music_ids.push_back(entity.music_id);
            });
        emit addPlaylist(add_playlist_music_ids, entities);
        });
    add_album_to_playlist_act->setIcon(qTheme.iconFromFont(0xe03b));

    auto* copy_album_act = action_map.addAction(tr("Copy album"), [album]() {
        QApplication::clipboard()->setText(album);
        });
    copy_album_act->setIcon(qTheme.iconFromFont(0xe14d));

    action_map.addAction(tr("Copy artist"), [artist]() {
        QApplication::clipboard()->setText(artist);
        });

    action_map.addSeparator();

    auto remove_select_album_act = action_map.addAction(tr("Remove select album"), [=]() {
        qDatabase.removeAlbum(album_id);
        refreshOnece();
        });
    remove_select_album_act->setIcon(qTheme.iconFromFont(0xe92b));

    action_map.exec(pt);
}

void AlbumView::enablePage(bool enable) {
    enable_page_ = enable;
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

void AlbumView::update() {
    model_.setQuery(Q_TEXT(R"(
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
    QString query(Q_TEXT(R"(
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

void AlbumView::processMeatadata(int64_t dir_last_write_time, const ForwardList<Metadata> &medata) {
    MetadataExtractAdapter::processMetadata(medata, nullptr, dir_last_write_time);
    emit loadCompleted();
}

void AlbumView::hideWidget() {
    if (!page_) {
        return;
    }
    page_->hide();
}
