#include <widget/albumview.h>

#include <widget/widget_shared.h>
#include <widget/scrolllabel.h>
#include <widget/appsettings.h>
#include <widget/database.h>
#include <widget/playlisttableview.h>
#include <widget/actionmap.h>
#include <widget/playlistpage.h>
#include <widget/appsettingnames.h>
#include <widget/processindicator.h>
#include <widget/str_utilts.h>
#include <widget/image_utiltis.h>
#include <widget/imagecache.h>
#include <widget/ui_utilts.h>
#include <widget/xprogressdialog.h>
#include <widget/playlistentity.h>

#include <base/scopeguard.h>

#include <thememanager.h>

#include <QGraphicsOpacityEffect>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QClipboard>
#include <QStandardPaths>
#include <QFileDialog>
#include <QPainterPath>
#include <QSqlError>
#include <QApplication>
#include <QHeaderView>

enum {
    INDEX_ALBUM = 0,
    INDEX_COVER,
    INDEX_ARTIST,
    INDEX_ALBUM_ID,
    INDEX_ARTIST_ID,
    INDEX_ARTIST_COVER_ID,
    INDEX_ALBUM_YEAR,
    INDEX_ALBUM_HEART,
};

const ConstLatin1String AlbumViewStyledDelegate::kAlbumCacheTag(qTEXT("album_thumbnail_"));

AlbumViewStyledDelegate::AlbumViewStyledDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
    , album_text_color_(Qt::black)
	, more_album_opt_button_(new QPushButton())
	, play_button_(new QPushButton()) {
    more_album_opt_button_->setStyleSheet(qTEXT("background-color: transparent"));
    play_button_->setStyleSheet(qTEXT("background-color: transparent"));
    mask_image_ = image_utils::roundDarkImage(qTheme.defaultCoverSize(),
                                              image_utils::kDarkAlpha, image_utils::kSmallImageRadius);
}

void AlbumViewStyledDelegate::setAlbumTextColor(QColor color) {
    album_text_color_ = color;
}

void AlbumViewStyledDelegate::enableAlbumView(bool enable) {
    enable_album_view_ = enable;
}

bool AlbumViewStyledDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) {
    if (!enable_album_view_) {
        return true;
    }

	const auto* ev = dynamic_cast<QMouseEvent*> (event);
    mouse_point_ = ev->pos();
	const auto current_cursor = QApplication::overrideCursor();

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
                emit showAlbumMenu(index, mouse_point_);
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

    auto album_id = indexValue(index, INDEX_ALBUM_ID).toInt();

    if (album_id == 0) {
        // Note: Qt6 當沒有可以顯示的內容album_id為0
        return;
    }

    auto album = indexValue(index, INDEX_ALBUM).toString();
    auto cover_id = indexValue(index, INDEX_COVER).toString();
    auto artist = indexValue(index, INDEX_ARTIST).toString();    
    auto album_year = indexValue(index, INDEX_ALBUM_YEAR).toInt();

    auto* style = option.widget ? option.widget->style() : QApplication::style();

    painter->setRenderHints(QPainter::Antialiasing, true);
    painter->setRenderHints(QPainter::SmoothPixmapTransform, true);
    painter->setRenderHints(QPainter::TextAntialiasing, true);

    if (show_mode_ == SHOW_YEAR) {
        artist = album_year <= 0 ? qTEXT("Unknown") : QString::number(album_year);
    }

    constexpr auto kPaddingSize = 5;

    const auto default_cover_size = qTheme.defaultCoverSize();
    const QRect cover_rect(option.rect.left() + kPaddingSize,
        option.rect.top() + kPaddingSize,
        default_cover_size.width(), 
        default_cover_size.height());

    auto album_artist_text_width = default_cover_size.width();

    QRect album_text_rect(option.rect.left() + kPaddingSize,
        option.rect.top() + default_cover_size.height() + 15,
        album_artist_text_width,
        15);
    QRect artist_text_rect(option.rect.left() + kPaddingSize,
        option.rect.top() + default_cover_size.height() + 30,
        default_cover_size.width(),
        20);

    painter->setPen(QPen(album_text_color_));

    auto f = painter->font();
    f.setPointSize(qTheme.fontSize(8));
    f.setBold(true);
    painter->setFont(f);

    if (playing_album_id_ > 0 && playing_album_id_ == album_id) {
        const QRect playing_state_icon_rect(
            option.rect.left() + default_cover_size.width() - 10,
            option.rect.top() + default_cover_size.height() + 15,
            kMoreIconSize, kMoreIconSize);

        album_artist_text_width -= kMoreIconSize;
        painter->drawPixmap(playing_state_icon_rect, qTheme.playingIcon().pixmap(QSize(kMoreIconSize, kMoreIconSize)));
    }

    QFontMetrics album_metrics(painter->font());
    painter->drawText(album_text_rect, Qt::AlignVCenter,
        album_metrics.elidedText(album, Qt::ElideRight, album_artist_text_width));

    painter->setPen(QPen(Qt::gray));
    f.setBold(false);
    painter->setFont(f);

    QFontMetrics artist_metrics(painter->font());
    painter->drawText(artist_text_rect, Qt::AlignVCenter,
        artist_metrics.elidedText(artist, Qt::ElideRight, default_cover_size.width() - kMoreIconSize));

    painter->drawPixmap(cover_rect, qImageCache.cover(kAlbumCacheTag, cover_id));

    bool hit_play_button = false;
    if (enable_album_view_
        && show_mode_ != SHOW_NORMAL
        && option.state & QStyle::State_MouseOver 
        && cover_rect.contains(mouse_point_)) {
        painter->drawPixmap(cover_rect, mask_image_);
        constexpr auto offset = (kIconSize / 2) - 10;

        const QRect button_rect(
            option.rect.left() + default_cover_size.width() / 2 - offset,
            option.rect.top() + default_cover_size.height() / 2 - offset,
            kIconSize, kIconSize);

        QStyleOptionButton button;
        button.rect = button_rect;
        button.icon = qTheme.playCircleIcon();
        button.state |= QStyle::State_Enabled;
        button.iconSize = QSize(kIconSize, kIconSize);
        style->drawControl(QStyle::CE_PushButton, &button, painter, play_button_.get());

        if (button_rect.contains(mouse_point_)) {
            QApplication::setOverrideCursor(Qt::PointingHandCursor);
            hit_play_button = true;
        }
    }

    QStyleOptionButton more_option_button;

    if (enable_album_view_
        && option.rect.contains(mouse_point_)
        && show_mode_ != SHOW_NORMAL) {
        const QRect more_button_rect(
            option.rect.left() + default_cover_size.width() - 10,
            option.rect.top() + default_cover_size.height() + 35,
            kMoreIconSize, kMoreIconSize);
        more_option_button.initFrom(more_album_opt_button_.get());
        more_option_button.rect = more_button_rect;
        more_option_button.icon = qTheme.fontIcon(Glyphs::ICON_MORE);
        more_option_button.state |= QStyle::State_Enabled;
        if (more_button_rect.contains(mouse_point_)) {
            more_option_button.state |= QStyle::State_Sunken;
            painter->setPen(qTheme.hoverColor());
            painter->setBrush(QBrush(qTheme.hoverColor()));
            painter->drawEllipse(more_button_rect);
        }
    }    

    if (more_album_opt_button_->isDefault()) {
        more_option_button.features = QStyleOptionButton::DefaultButton;
    }
    more_option_button.iconSize = QSize(kMoreIconSize, kMoreIconSize);
    style->drawControl(QStyle::CE_PushButton, &more_option_button, painter, more_album_opt_button_.get());
    if (!hit_play_button) {
        QApplication::restoreOverrideCursor();
    }    
}

QSize AlbumViewStyledDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    auto result = QStyledItemDelegate::sizeHint(option, index);
    const auto default_cover = qTheme.defaultCoverSize();
    result.setWidth(default_cover.width() + 30);
    result.setHeight(default_cover.height() + 85);
    return result;
}

AlbumViewPage::AlbumViewPage(QWidget* parent)
    : QFrame(parent) {
    setObjectName(qTEXT("albumViewPage"));
    setFrameStyle(QFrame::StyledPanel);

    auto* default_layout = new QVBoxLayout(this);
    default_layout->setSpacing(0);
    default_layout->setContentsMargins(0, 5, 0, 0);

    close_button_ = new QPushButton(this);
    close_button_->setObjectName(qTEXT("albumViewPageCloseButton"));
    close_button_->setCursor(Qt::PointingHandCursor);
    close_button_->setAttribute(Qt::WA_TranslucentBackground);
    close_button_->setFixedSize(qTheme.titleButtonIconSize() * 1.5);
    close_button_->setIconSize(qTheme.titleButtonIconSize() * 1.5);
    close_button_->setIcon(qTheme.fontIcon(Glyphs::ICON_CLOSE_WINDOW, qTheme.themeColor()));

    auto* hbox_layout = new QHBoxLayout();
    hbox_layout->setSpacing(0);
    hbox_layout->setContentsMargins(10, 10, 0, 0);

    auto* button_spacer = new QSpacerItem(20, 5, QSizePolicy::Expanding, QSizePolicy::Expanding);
    hbox_layout->addWidget(close_button_);
    hbox_layout->addSpacerItem(button_spacer);

    page_ = new PlaylistPage(this);
    page_->playlist()->setPlaylistId(kDefaultAlbumPlaylistId, kAppSettingAlbumPlaylistColumnName);
    page_->playlist()->setOtherPlaylist(kDefaultPlaylistId);

    default_layout->addLayout(hbox_layout);
    default_layout->addWidget(page_);
    default_layout->setStretch(0, 0);
    default_layout->setStretch(1, 1);

    (void)QObject::connect(close_button_, &QPushButton::clicked, [this]() {
        emit leaveAlbumView();
    });

    page_->playlist()->disableDelete();
    page_->playlist()->disableLoadFile();

    if (qTheme.themeColor() == ThemeColor::LIGHT_THEME) {
        auto* shadow_effect = new QGraphicsDropShadowEffect();
        shadow_effect->setOffset(0, 0);
        shadow_effect->setColor(QColor(0, 0, 0, 80));
        shadow_effect->setBlurRadius(15);
        setGraphicsEffect(shadow_effect);
    }
}

void AlbumViewPage::onCurrentThemeChanged(ThemeColor theme_color) {
    close_button_->setIcon(qTheme.fontIcon(Glyphs::ICON_CLOSE_WINDOW, theme_color));
}

void AlbumViewPage::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
}

void AlbumViewPage::setPlaylistMusic(const QString& album, int32_t album_id, const QString &cover_id, int32_t album_heart) {
    if (qTheme.themeColor() == ThemeColor::LIGHT_THEME) {
        setStyleSheet(qSTR(
            R"(
           QFrame#albumViewPage {
		        background-color: %1;
                border-radius: 4px;
				border: 1px solid #C9CDD0;
           }
        )"
        ).arg(qTheme.linearGradientStyle()));
    } else {
        setStyleSheet(qSTR(
            R"(
           QFrame#albumViewPage {
		        background-color: %1;
                border-radius: 4px;
           }
        )"
        ).arg(qTheme.linearGradientStyle()));
    }

    QList<int32_t> add_playlist_music_ids;

    page_->playlist()->horizontalHeader()->hide();
    page_->playlist()->setAlternatingRowColors(false);
    page_->playlist()->removeAll();

    qMainDb.forEachAlbumMusic(album_id,
        [&add_playlist_music_ids](const PlayListEntity& entity) mutable {
            add_playlist_music_ids.push_back(entity.music_id);
        });

    qMainDb.addMusicToPlaylist(add_playlist_music_ids,
        page_->playlist()->playlistId());

    page_->setAlbumId(album_id, album_heart);
    page_->playlist()->reload();
    page_->playlist()->disableDelete();
    page_->title()->setText(album);
    page_->onSetCoverById(cover_id);

    if (const auto album_stats = qMainDb.getAlbumStats(album_id)) {
        page_->format()->setText(qTR("%1 Songs, %2, %3, %4")
            .arg(QString::number(album_stats.value().songs))
            .arg(formatDuration(album_stats.value().durations))
            .arg(QString::number(album_stats.value().year))
            .arg(formatBytes(album_stats.value().file_size))
        );
    }

    page_->show();
}

AlbumView::AlbumView(QWidget* parent)
    : QListView(parent)
    , page_(nullptr)
	, styled_delegate_(new AlbumViewStyledDelegate(this))
    , animation_(nullptr)
	, model_(this)
    , proxy_model_(new PlayListTableFilterProxyModel(this)) {
    proxy_model_->addFilterByColumn(INDEX_ALBUM);
    proxy_model_->setSourceModel(&model_);
    setModel(proxy_model_);

    setUniformItemSizes(true);
    setDragEnabled(false);
    setSelectionRectVisible(false);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setWrapping(true);
    setFlow(QListView::LeftToRight);
    setViewMode(QListView::IconMode);
    setResizeMode(QListView::Adjust);
    setFrameStyle(QFrame::StyledPanel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setItemDelegate(styled_delegate_);
    setAutoScroll(false);
    viewport()->setAttribute(Qt::WA_StaticContents);
    setMouseTracking(true);       

    (void)QObject::connect(styled_delegate_, &AlbumViewStyledDelegate::showAlbumMenu, [this](auto index, auto pt) {
        showMenu(pt);
        });

    (void)QObject::connect(styled_delegate_, &AlbumViewStyledDelegate::enterAlbumView, [this](auto index) {
        albumViewPage();

        auto album = indexValue(index, INDEX_ALBUM).toString();
        auto cover_id = indexValue(index, INDEX_COVER).toString();
        auto artist = indexValue(index, INDEX_ARTIST).toString();
        auto album_id = indexValue(index, INDEX_ALBUM_ID).toInt();
        auto artist_id = indexValue(index, INDEX_ARTIST_ID).toInt();
        auto artist_cover_id = indexValue(index, INDEX_ARTIST_COVER_ID).toString();
        auto album_heart = indexValue(index, INDEX_ALBUM_HEART).toInt();

        const auto list_view_rect = this->rect();
        page_->setPlaylistMusic(album, album_id, cover_id, album_heart);
        page_->setFixedSize(QSize(list_view_rect.size().width() - 2, list_view_rect.height()));

        if (enable_page_) {
            page_->show();
        }

        verticalScrollBar()->hide();
        emit clickedAlbum(album, album_id, cover_id);
        }); 

    setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(this, &QTableView::customContextMenuRequested, [this](auto pt) {
        showAlbumViewMenu(pt);
    });

    verticalScrollBar()->setStyleSheet(qTEXT(
        "QScrollBar:vertical { width: 6px; }"
    ));
}

void AlbumView::setPlayingAlbumId(int32_t album_id) {
    styled_delegate_->setPlayingAlbumId(album_id);
}

void AlbumView::showAlbumViewMenu(const QPoint& pt) {
	const auto index = indexAt(pt);

    ActionMap<AlbumView> action_map(this);

    if (index.isValid()) {
        const auto album = indexValue(index, INDEX_ALBUM).toString();
        const auto artist = indexValue(index, INDEX_ARTIST).toString();

        auto* copy_album_act = action_map.addAction(qTR("Copy album"), [album]() {
            QApplication::clipboard()->setText(album);
            });
        copy_album_act->setIcon(qTheme.fontIcon(Glyphs::ICON_COPY));

        action_map.addAction(qTR("Copy artist"), [artist]() {
            QApplication::clipboard()->setText(artist);
            });
    }   

    XAMP_ON_SCOPE_EXIT(
        action_map.exec(pt);
    );

	const auto show_mode = dynamic_cast<AlbumViewStyledDelegate*>(itemDelegate())->showModes();
    if (show_mode == SHOW_NORMAL) {
        return;
    }

    action_map.addSeparator();

    auto remove_album = [this]() {
        if (!model_.rowCount()) {
            return;
        }

        const auto process_dialog = makeProgressDialog(
            qTR("Remove all album"),
            kEmptyString,
            qTR("Cancel"));
        
        if (!qMainDb.transaction()) {
            return;
        }

        try {
            QList<int32_t> albums;
            qMainDb.forEachAlbum([&albums](auto album_id) {
                albums.push_back(album_id);
            });

            process_dialog->setRange(0, albums.size() + 1);
            int32_t count = 0;
            process_dialog->show();
            Q_FOREACH(const auto album_id, albums) {
                qMainDb.removeAlbum(album_id);
                process_dialog->setValue(count++ * 100 / albums.size() + 1);
                qApp->processEvents();
            }
            qMainDb.removeAllArtist();
            process_dialog->setValue(100);
            qMainDb.commit();
            update();
            emit removeAll();
            qImageCache.clear();
        }
        catch (...) {
            qMainDb.rollback();
        }
    };

    auto* load_file_act = action_map.addAction(qTR("Load local file"), [this]() {
        getOpenMusicFileName(this, [this](const auto& file_name) {
            append(file_name);
            });
        });
    load_file_act->setIcon(qTheme.fontIcon(Glyphs::ICON_LOAD_FILE));

    auto* load_dir_act = action_map.addAction(qTR("Load file directory"), [this]() {
        const auto dir_name = getExistingDirectory(this);
        if (dir_name.isEmpty()) {
            return;
        }
        append(dir_name);
        });
    load_dir_act->setIcon(qTheme.fontIcon(Glyphs::ICON_FOLDER));
    action_map.addSeparator();
    auto* remove_all_album_act = action_map.addAction(qTR("Remove all album"), [=]() {
        remove_album();
        });
    remove_all_album_act->setIcon(qTheme.fontIcon(Glyphs::ICON_REMOVE_ALL));    
}

void AlbumView::showMenu(const QPoint &pt) {
	const auto index = indexAt(pt);

    ActionMap<AlbumView> action_map(this);

    auto album = indexValue(index, INDEX_ALBUM).toString();
    auto artist = indexValue(index, INDEX_ARTIST).toString();
    auto album_id = indexValue(index, INDEX_ALBUM_ID).toInt();
    auto artist_id = indexValue(index, INDEX_ARTIST_ID).toInt();
    auto artist_cover_id = indexValue(index, INDEX_ARTIST_COVER_ID).toString();

    auto* add_album_to_playlist_act = action_map.addAction(qTR("Add album to playlist"), [album_id, this]() {
        QList<PlayListEntity> entities;
        QList<int32_t> add_playlist_music_ids;
        qMainDb.forEachAlbumMusic(album_id,
            [&entities, &add_playlist_music_ids](const PlayListEntity& entity) mutable {
                if (entity.track_loudness == 0.0) {
                    entities.push_back(entity);
                }
                add_playlist_music_ids.push_back(entity.music_id);
            });
        emit addPlaylist(add_playlist_music_ids, entities);
        });
    add_album_to_playlist_act->setIcon(qTheme.fontIcon(Glyphs::ICON_PLAYLIST));

    auto* copy_album_act = action_map.addAction(qTR("Copy album"), [album]() {
        QApplication::clipboard()->setText(album);
    });
    copy_album_act->setIcon(qTheme.fontIcon(Glyphs::ICON_COPY));

    action_map.addAction(qTR("Copy artist"), [artist]() {
        QApplication::clipboard()->setText(artist);
    });

    action_map.addSeparator();

    const auto remove_select_album_act = action_map.addAction(qTR("Remove select album"), [album_id, this]() {
        qMainDb.removeAlbum(album_id);
        reload();
    });
    remove_select_album_act->setIcon(qTheme.fontIcon(Glyphs::ICON_REMOVE_ALL));

    action_map.exec(pt);
}

void AlbumView::enablePage(bool enable) {
    enable_page_ = enable;
    styled_delegate_->enableAlbumView(enable);
}

void AlbumView::onThemeColorChanged(QColor backgroundColor, QColor color) {
    dynamic_cast<AlbumViewStyledDelegate*>(itemDelegate())->setAlbumTextColor(color);
}

void AlbumView::setShowMode(ShowModes mode) {
    dynamic_cast<AlbumViewStyledDelegate*>(itemDelegate())->setShowMode(mode);
}

void AlbumView::filterByArtistId(int32_t artist_id) {
    last_query_ = qSTR(R"(
    SELECT
        album,
        albums.coverId,
        artist,
        albums.albumId,
        artists.artistId,
        artists.coverId as artistCover,
        albums.year,
        albums.heart
    FROM
        albumArtist
    LEFT JOIN
        albums ON albums.albumId = albumArtist.albumId
    LEFT JOIN
        artists ON artists.artistId = albumArtist.artistId
    WHERE
        (artists.artistId = %1) AND (albums.isPodcast = 0)
	GROUP BY 
		albums.albumId
    ORDER BY
        albums.year DESC
    )").arg(artist_id);
    setShowMode(SHOW_YEAR);
}

void AlbumView::filterCategories(const QString & category) {
    last_query_ = qSTR(R"(
SELECT
    albums.album,
    albums.coverId,
    artists.artist,
    albums.albumId,
    artists.artistId,
    artists.coverId as artistCover,
    albums.year,
    albums.heart
FROM
    albums
LEFT 
	JOIN artists ON artists.artistId = albums.artistId
LEFT 
	JOIN albumCategories ON albumCategories.albumId = albums.albumId
WHERE 
	albumCategories.category = '%1'
ORDER BY
    albums.album DESC
    )").arg(category);
    setShowMode(SHOW_ARTIST);
}

void AlbumView::sortYears() {
    last_query_ = qSTR(R"(
SELECT
    albums.album,
    albums.coverId,
    artists.artist,
    albums.albumId,
    artists.artistId,
    artists.coverId as artistCover,
    albums.year,
    albums.heart
FROM
    albums
LEFT 
	JOIN artists ON artists.artistId = albums.artistId
ORDER BY
    albums.year DESC
    )");
    setShowMode(SHOW_ARTIST);
}

void AlbumView::filterYears(const QSet<QString>& years) {
    QStringList year_list;
    Q_FOREACH(auto & c, years) {
        year_list.append(qSTR("'%1'").arg(c));
    }
    last_query_ = qSTR(R"(
SELECT
    albums.album,
    albums.coverId,
    artists.artist,
    albums.albumId,
    artists.artistId,
    artists.coverId as artistCover,
    albums.year,
    albums.heart
FROM
    albums
LEFT 
	JOIN artists ON artists.artistId = albums.artistId
WHERE 
	albums.year IN (%1)
ORDER BY
    albums.album DESC
    )").arg(year_list.join(","));
    setShowMode(SHOW_ARTIST);
}

void AlbumView::filterCategories(const QSet<QString>& category) {
    QStringList categories;
    Q_FOREACH(auto & c, category) {
        categories.append(qSTR("'%1'").arg(c));
	}
    last_query_ = qSTR(R"(
SELECT
    albums.album,
    albums.coverId,
    artists.artist,
    albums.albumId,
    artists.artistId,
    artists.coverId as artistCover,
    albums.year,
    albums.heart
FROM
    albums
LEFT 
	JOIN artists ON artists.artistId = albums.artistId
LEFT 
	JOIN albumCategories ON albumCategories.albumId = albums.albumId
WHERE 
	albumCategories.category IN (%1)
ORDER BY
    albums.album DESC
    )").arg(categories.join(","));
    setShowMode(SHOW_ARTIST);
}

void AlbumView::showAll() {
    last_query_ = qSTR(R"(
SELECT
    albums.album,
    albums.coverId,
    artists.artist,
    albums.albumId,
    artists.artistId,
    artists.coverId as artistCover,
    albums.year,
    albums.heart
FROM
    albums
LEFT 
	JOIN artists ON artists.artistId = albums.artistId
WHERE 
	albums.storeType = -1 OR albums.storeType = -2
ORDER BY
    albums.album DESC
    )");   
    setShowMode(SHOW_ARTIST);
}

void AlbumView::search(const QString& keyword) {
    const QRegularExpression reg_exp(keyword, QRegularExpression::CaseInsensitiveOption);
    proxy_model_->addFilterByColumn(INDEX_ALBUM);
    proxy_model_->addFilterByColumn(INDEX_ARTIST);
    proxy_model_->setFilterRegularExpression(reg_exp);
    setShowMode(SHOW_ARTIST);
}

AlbumViewPage* AlbumView::albumViewPage() {
    if (!page_) {
        page_ = new AlbumViewPage(this);
        page_->hide();

        const auto list_view_rect = this->rect();
        page_->setFixedSize(QSize(list_view_rect.size().width() - 2, list_view_rect.height()));

        (void)QObject::connect(page_,
            &AlbumViewPage::clickedArtist,
            this,
            &AlbumView::clickedArtist);
        (void)QObject::connect(page_->playlistPage()->playlist(),
            &PlayListTableView::updatePlayingState,
            this,
            [this](auto entity, auto playing_state) {
                styled_delegate_->setPlayingAlbumId(playing_state == PlayingState::PLAY_CLEAR ? -1 : entity.album_id);
            });
        (void)QObject::connect(page_, &AlbumViewPage::leaveAlbumView, [this]() {
            verticalScrollBar()->show();
            page_->hide();
            });
        (void)QObject::connect(verticalScrollBar(), &QAbstractSlider::valueChanged, [this](auto value) {
            if (value == verticalScrollBar()->maximum()) {
                model_.fetchMore();
            }
            });
    }
    return page_;
}

void AlbumView::reload() {
    if (last_query_.isEmpty()) {
        showAll();
    }
    model_.setQuery(last_query_, qMainDb.database());
    if (model_.lastError().type() != QSqlError::NoError) {
        XAMP_LOG_DEBUG("SqlException: {}", model_.lastError().text().toStdString());
    }
}

void AlbumView::onCurrentThemeChanged(ThemeColor theme_color) {
    if (page_ != nullptr) {
        page_->onCurrentThemeChanged(theme_color);
    }    
}

void AlbumView::append(const QString& file_name) {
    emit extractFile(file_name, -1);
}

void AlbumView::hideWidget() {
    if (!page_) {
        return;
    }
    page_->hide();
}

void AlbumView::resizeEvent(QResizeEvent* event) {
    if (page_ != nullptr) {
        if (!page_->isHidden()) {
            const auto list_view_rect = this->rect();
            page_->setFixedSize(QSize(list_view_rect.size().width() - 2, list_view_rect.height()));
        }
    }    
    QListView::resizeEvent(event);
}
