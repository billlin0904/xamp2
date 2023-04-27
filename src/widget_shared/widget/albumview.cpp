#include <widget/albumview.h>

#include <widget/widget_shared.h>
#include <widget/scrolllabel.h>
#include <widget/appsettings.h>
#include <widget/database.h>
#include <widget/playlisttableview.h>
#include <widget/actionmap.h>
#include <widget/playlistpage.h>
#include <widget/appsettingnames.h>
#include <widget/xmessagebox.h>
#include <widget/processindicator.h>
#include <widget/str_utilts.h>
#include <widget/image_utiltis.h>
#include <widget/imagecache.h>
#include <widget/ui_utilts.h>
#include <widget/xprogressdialog.h>
#include <widget/albumentity.h>
#include <widget/xmessage.h>

#include <widget/widget_shared.h>

#include <thememanager.h>

#include <QGraphicsOpacityEffect>
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

AlbumViewStyledDelegate::AlbumViewStyledDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
    , text_color_(Qt::black)
	, more_album_opt_button_(new QPushButton())
	, play_button_(new QPushButton()) {
    more_album_opt_button_->setStyleSheet(qTEXT("background-color: transparent"));
    play_button_->setStyleSheet(qTEXT("background-color: transparent"));
    mask_image_ = image_utils::RoundDarkImage(qTheme.GetDefaultCoverSize(), 
        80, image_utils::kSmallImageRadius);
}

void AlbumViewStyledDelegate::SetTextColor(QColor color) {
    text_color_ = color;
}

void AlbumViewStyledDelegate::EnableAlbumView(bool enable) {
    enable_album_view_ = enable;
}

void AlbumViewStyledDelegate::LoadCoverCache() {
    qDatabase.ForEachAlbumCover([](const auto& cover_id) {
        GetCover(qTEXT("album_") + cover_id);
        }, kMaxAlbumRoundedImageCacheSize);
}

QPixmap AlbumViewStyledDelegate::GetCover(const QString& cover_id) {
    return qPixmapCache.GetOrAdd(qTEXT("album_") + cover_id, [cover_id]() {
        return image_utils::RoundImage(
            image_utils::ResizeImage(qPixmapCache.GetOrDefault(cover_id), qTheme.GetDefaultCoverSize(), true),
            image_utils::kSmallImageRadius);
        });
}

bool AlbumViewStyledDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) {
    if (!enable_album_view_) {
        return true;
    }

	const auto* ev = static_cast<QMouseEvent*> (event);
    mouse_point_ = ev->pos();
	const auto current_cursor = QApplication::overrideCursor();

    const auto default_cover_size = qTheme.GetDefaultCoverSize();
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
                    emit EnterAlbumView(index);
                }
            }
            if (more_button_rect.contains(mouse_point_)) {
                emit ShowAlbumMenu(index, mouse_point_);
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

    auto* style = option.widget ? option.widget->style() : QApplication::style();

    painter->setRenderHints(QPainter::Antialiasing, true);
    painter->setRenderHints(QPainter::SmoothPixmapTransform, true);
    painter->setRenderHints(QPainter::TextAntialiasing, true);

    auto album = GetIndexValue(index, INDEX_ALBUM).toString();
    auto cover_id = GetIndexValue(index, INDEX_COVER).toString();
    auto artist = GetIndexValue(index, INDEX_ARTIST).toString();
    auto album_id = GetIndexValue(index, INDEX_ALBUM_ID).toInt();
    auto album_year = GetIndexValue(index, INDEX_ALBUM_YEAR).toInt();

    if (show_mode_ == SHOW_YEAR) {
        artist = album_year <= 0 ? qTEXT("Unknown") : QString::number(album_year);
    }

    const auto default_cover_size = qTheme.GetDefaultCoverSize();
    const QRect cover_rect(option.rect.left() + 10,
        option.rect.top() + 10,
        default_cover_size.width(), 
        default_cover_size.height());

    auto album_artist_text_width = default_cover_size.width();

    QRect album_text_rect(option.rect.left() + 10,
        option.rect.top() + default_cover_size.height() + 15,
        album_artist_text_width,
        15);
    QRect artist_text_rect(option.rect.left() + 10,
        option.rect.top() + default_cover_size.height() + 35,
        default_cover_size.width(),
        15);

    painter->setPen(QPen(text_color_));

    auto f = painter->font();
    f.setPointSize(qTheme.GetFontSize(8));
    f.setBold(true);
    painter->setFont(f);

    if (playing_album_id_ > 0 && playing_album_id_ == album_id) {
        const QRect playing_state_icon_rect(
            option.rect.left() + default_cover_size.width() - 10,
            option.rect.top() + default_cover_size.height() + 15,
            kMoreIconSize, kMoreIconSize);

        album_artist_text_width -= kMoreIconSize;
        painter->drawPixmap(playing_state_icon_rect, qTheme.GetPlayingIcon().pixmap(QSize(kMoreIconSize, kMoreIconSize)));
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

    
    painter->drawPixmap(cover_rect, GetCover(cover_id));

    bool hit_play_button = false;
    if (enable_album_view_ && option.state & QStyle::State_MouseOver && cover_rect.contains(mouse_point_)) {
        painter->drawPixmap(cover_rect, mask_image_);
        constexpr auto offset = (kIconSize / 2) - 10;

        const QRect button_rect(
            option.rect.left() + default_cover_size.width() / 2 - offset,
            option.rect.top() + default_cover_size.height() / 2 - offset,
            kIconSize, kIconSize);

        QStyleOptionButton button;
        button.rect = button_rect;
        button.icon = qTheme.GetPlayCircleIcon();
        button.state |= QStyle::State_Enabled;
        button.iconSize = QSize(kIconSize, kIconSize);
        style->drawControl(QStyle::CE_PushButton, &button, painter, play_button_.get());

        if (button_rect.contains(mouse_point_)) {
            QApplication::setOverrideCursor(Qt::PointingHandCursor);
            hit_play_button = true;
        }
    }

    QStyleOptionButton more_option_button;

    if (enable_album_view_) {
        const QRect more_button_rect(
            option.rect.left() + default_cover_size.width() - 10,
            option.rect.top() + default_cover_size.height() + 35,
            kMoreIconSize, kMoreIconSize);
        more_option_button.initFrom(more_album_opt_button_.get());
        more_option_button.rect = more_button_rect;
        more_option_button.icon = qTheme.GetFontIcon(Glyphs::ICON_MORE);
        more_option_button.state |= QStyle::State_Enabled;
        if (more_button_rect.contains(mouse_point_)) {
            more_option_button.state |= QStyle::State_Sunken;
            painter->setPen(qTheme.GetHoverColor());
            painter->setBrush(QBrush(qTheme.GetHoverColor()));
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
    const auto default_cover = qTheme.GetDefaultCoverSize();
    result.setWidth(default_cover.width() + 30);
    result.setHeight(default_cover.height() + 80);
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
    close_button_->setStyleSheet(qTEXT(R"(
                                         QPushButton#albumViewPageCloseButton {
                                         border-radius: 5px;        
                                         background-color: gray;
                                         }
										 QPushButton#albumViewPageCloseButton:hover {
                                         color: white;
                                         background-color: darkgray;                                         
                                         }
                                         )"));
    close_button_->setAttribute(Qt::WA_TranslucentBackground);
    close_button_->setFixedSize(qTheme.GetTitleButtonIconSize() * 2);
    close_button_->setIconSize(qTheme.GetTitleButtonIconSize());
    close_button_->setIcon(qTheme.GetFontIcon(Glyphs::ICON_CLOSE_WINDOW, ThemeColor::DARK_THEME));

    auto* hbox_layout = new QHBoxLayout();
    hbox_layout->setSpacing(0);
    hbox_layout->setContentsMargins(15, 15, 0, 0);

    auto* button_spacer = new QSpacerItem(20, 10, QSizePolicy::Expanding, QSizePolicy::Expanding);
    hbox_layout->addWidget(close_button_);
    hbox_layout->addSpacerItem(button_spacer);

    page_ = new PlaylistPage(this);
    page_->playlist()->SetPlaylistId(kDefaultAlbumPlaylistId, kAppSettingAlbumPlaylistColumnName);

    default_layout->addLayout(hbox_layout);
    default_layout->addWidget(page_);
    default_layout->setStretch(0, 0);
    default_layout->setStretch(1, 1);

    (void)QObject::connect(close_button_, &QPushButton::clicked, [this]() {
        emit LeaveAlbumView();
    });

    setStyleSheet(qTEXT("QFrame#albumViewPage { background-color: transparent; }"));
    page_->playlist()->DisableDelete();
    page_->playlist()->DisableLoadFile();

    auto * fade_effect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(fade_effect);
}

void AlbumViewPage::OnCurrentThemeChanged(ThemeColor theme_color) {
    close_button_->setIcon(qTheme.GetFontIcon(Glyphs::ICON_CLOSE_WINDOW, ThemeColor::DARK_THEME));
}

void AlbumViewPage::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    QLinearGradient gradient(0, 0, 0, height());
    qTheme.SetLinearGradient(gradient);
    painter.fillRect(rect(), gradient);
}

void AlbumViewPage::SetPlaylistMusic(const QString& album, int32_t album_id, const QString &cover_id, int32_t album_heart) {
    ForwardList<int32_t> add_playlist_music_ids;

    page_->playlist()->RemoveAll();

    qDatabase.ForEachAlbumMusic(album_id,
        [&add_playlist_music_ids](const PlayListEntity& entity) mutable {
            add_playlist_music_ids.push_front(entity.music_id);
        });

    IGNORE_DB_EXCEPTION(qDatabase.AddMusicToPlaylist(add_playlist_music_ids,
        page_->playlist()->GetPlaylistId()))

    page_->SetAlbumId(album_id, album_heart);
    page_->playlist()->Reload();
    page_->title()->setText(album);
    page_->SetCoverById(cover_id);

    if (const auto album_stats = qDatabase.GetAlbumStats(album_id)) {
        page_->format()->setText(tr("%1 Songs, %2, %3, %4")
            .arg(QString::number(album_stats.value().songs))
            .arg(FormatDuration(album_stats.value().durations))
            .arg(QString::number(album_stats.value().year))
            .arg(FormatBytes(album_stats.value().file_size))
        );
    }

    page_->show();
}

AlbumView::AlbumView(QWidget* parent)
    : QListView(parent)
    , page_(new AlbumViewPage(this))
	, styled_delegate_(new AlbumViewStyledDelegate(this))
	, model_(this) {
    setModel(&model_);    
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

    page_->hide();

    (void)QObject::connect(page_,
                            &AlbumViewPage::ClickedArtist,
                            this,
                            &AlbumView::ClickedArtist);

    (void)QObject::connect(page_->playlistPage()->playlist(),
        &PlayListTableView::UpdatePlayingState,
        this,
        [this](auto entity, auto playing_state) {
            styled_delegate_->SetPlayingAlbumId(playing_state == PlayingState::PLAY_CLEAR ? -1 : entity.album_id);
        });

    (void)QObject::connect(styled_delegate_, &AlbumViewStyledDelegate::ShowAlbumMenu, [this](auto index, auto pt) {
        ShowMenu(pt);
        });

    (void)QObject::connect(styled_delegate_, &AlbumViewStyledDelegate::EnterAlbumView, [this](auto index) {
        auto album = GetIndexValue(index, INDEX_ALBUM).toString();
        auto cover_id = GetIndexValue(index, INDEX_COVER).toString();
        auto artist = GetIndexValue(index, INDEX_ARTIST).toString();
        auto album_id = GetIndexValue(index, INDEX_ALBUM_ID).toInt();
        auto artist_id = GetIndexValue(index, INDEX_ARTIST_ID).toInt();
        auto artist_cover_id = GetIndexValue(index, INDEX_ARTIST_COVER_ID).toString();    
        auto album_heart = GetIndexValue(index, INDEX_ALBUM_HEART).toInt();

        const auto list_view_rect = this->rect();
        page_->SetPlaylistMusic(album, album_id, cover_id, album_heart);
        page_->setFixedSize(QSize(list_view_rect.size().width() - 15, list_view_rect.height() + 25));
        page_->move(QPoint(list_view_rect.x() + 3, 0));

        if (enable_page_) {
            ShowPageAnimation();
            page_->show();
        }

        verticalScrollBar()->hide();
        emit ClickedAlbum(album, album_id, cover_id);
        });

    (void)QObject::connect(page_, &AlbumViewPage::LeaveAlbumView, [this]() {
        verticalScrollBar()->show();
		HidePageAnimation();
        });

    setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(this, &QTableView::customContextMenuRequested, [this](auto pt) {
        ShowAlbumViewMenu(pt);
    });

    setStyleSheet(qTEXT("background-color: transparent"));

    verticalScrollBar()->setStyleSheet(qTEXT(
        "QScrollBar:vertical { width: 6px; }"
    ));

    auto * fade_effect = page_->graphicsEffect();
    animation_ = new QPropertyAnimation(fade_effect, "opacity");
    QObject::connect(animation_, &QPropertyAnimation::finished, [this]() {
        if (hide_page_) {
            page_->hide();
        }
    });
    
    Refresh();

    QObject::connect(verticalScrollBar(), &QAbstractSlider::valueChanged, [this](int value) {
        if (value == verticalScrollBar()->maximum()) {
            model_.fetchMore();
        }
    });
    update();
}

void AlbumView::HidePageAnimation() {
    animation_->setStartValue(1.0);
    animation_->setEndValue(0.0);
    animation_->setDuration(kPageAnimationDurationMs);
    animation_->setEasingCurve(QEasingCurve::OutCubic);
    animation_->start();
    hide_page_ = true;
}

void AlbumView::ShowPageAnimation() {
    animation_->setStartValue(0.01);
    animation_->setEndValue(1.0);
    animation_->setDuration(kPageAnimationDurationMs);
    animation_->setEasingCurve(QEasingCurve::OutCubic);
    animation_->start();
    hide_page_ = false;
}

void AlbumView::SetPlayingAlbumId(int32_t album_id) {
    styled_delegate_->SetPlayingAlbumId(album_id);
}

void AlbumView::ShowAlbumViewMenu(const QPoint& pt) {
    auto index = indexAt(pt);

    ActionMap<AlbumView> action_map(this);

    auto removeAlbum = [=]() {
        if (!model_.rowCount()) {
            return;
        }

        const auto button = XMessageBox::ShowYesOrNo(tr("Remove all album?"));
        if (button == QDialogButtonBox::Yes) {
            const QScopedPointer<ProcessIndicator> indicator(new ProcessIndicator(this));
            indicator->StartAnimation();
            try {
                qDatabase.ClearPendingPlaylist();
                QList<int32_t> albums;
                qDatabase.ForEachAlbum([&albums](auto album_id) {
                    albums.push_back(album_id);                    
                    });
                Q_FOREACH(auto album_id, albums) {
                    qDatabase.RemoveAlbum(album_id);
                }                
                qDatabase.RemoveAllArtist();
                update();
                emit RemoveAll();
                qPixmapCache.Clear();
            }
            catch (...) {
            }
        }
    };

    auto* load_file_act = action_map.AddAction(tr("Load local file"), [this]() {
        const auto file_name = QFileDialog::getOpenFileName(this,
            tr("Open file"),
            AppSettings::GetMyMusicFolderPath(),
            tr("Music Files ") + GetFileDialogFileExtensions(),
            nullptr);
        if (file_name.isEmpty()) {
            return;
        }
        append(file_name);
        });
    load_file_act->setIcon(qTheme.GetFontIcon(Glyphs::ICON_LOAD_FILE));

    auto* load_dir_act = action_map.AddAction(tr("Load file directory"), [this]() {
        const auto dir_name = GetExistingDirectory(this,
        tr("Select a directory"),
        AppSettings::GetMyMusicFolderPath());
        if (dir_name.isEmpty()) {
            return;
        }
        append(dir_name);
        });
    load_dir_act->setIcon(qTheme.GetFontIcon(Glyphs::ICON_FOLDER));
    action_map.AddSeparator();
    auto* remove_all_album_act = action_map.AddAction(tr("Remove all album"), [=]() {
        removeAlbum();
        });
    remove_all_album_act->setIcon(qTheme.GetFontIcon(Glyphs::ICON_REMOVE_ALL));

    action_map.exec(pt);
}

void AlbumView::ShowMenu(const QPoint &pt) {
    auto index = indexAt(pt);

    ActionMap<AlbumView> action_map(this);

    auto album = GetIndexValue(index, INDEX_ALBUM).toString();
    auto artist = GetIndexValue(index, INDEX_ARTIST).toString();
    auto album_id = GetIndexValue(index, INDEX_ALBUM_ID).toInt();
    auto artist_id = GetIndexValue(index, INDEX_ARTIST_ID).toInt();
    auto artist_cover_id = GetIndexValue(index, INDEX_ARTIST_COVER_ID).toString();

    auto* add_album_to_playlist_act = action_map.AddAction(tr("Add album to playlist"), [=]() {
        ForwardList<PlayListEntity> entities;
		ForwardList<int32_t> add_playlist_music_ids;
        qDatabase.ForEachAlbumMusic(album_id,
            [&entities, &add_playlist_music_ids](const PlayListEntity& entity) mutable {
                if (entity.track_loudness == 0.0) {
                    entities.push_front(entity);
                }
                add_playlist_music_ids.push_front(entity.music_id);
            });
        emit AddPlaylist(add_playlist_music_ids, entities);
        });
    add_album_to_playlist_act->setIcon(qTheme.GetFontIcon(Glyphs::ICON_PLAYLIST));

    auto* copy_album_act = action_map.AddAction(tr("Copy album"), [album]() {
        QApplication::clipboard()->setText(album);
    });
    copy_album_act->setIcon(qTheme.GetFontIcon(Glyphs::ICON_COPY));

    action_map.AddAction(tr("Copy artist"), [artist]() {
        QApplication::clipboard()->setText(artist);
    });

    action_map.AddSeparator();

    auto remove_select_album_act = action_map.AddAction(tr("Remove select album"), [=]() {
        const auto button = XMessageBox::ShowYesOrNo(tr("Remove the album?"));
		if (button == QDialogButtonBox::Yes) {
            qDatabase.RemoveAlbum(album_id);
            Refresh();
		}
    });
    remove_select_album_act->setIcon(qTheme.GetFontIcon(Glyphs::ICON_REMOVE_ALL));

    action_map.exec(pt);
}

void AlbumView::EnablePage(bool enable) {
    enable_page_ = enable;
    styled_delegate_->EnableAlbumView(enable);
}

void AlbumView::OnThemeChanged(QColor backgroundColor, QColor color) {
    dynamic_cast<AlbumViewStyledDelegate*>(itemDelegate())->SetTextColor(color);
}

void AlbumView::FilterByArtistId(int32_t artist_id) {
    last_query_ = qSTR(R"(
    SELECT
        album,
        albums.coverId,
        artist,
        albums.albumId,
        artists.artistId,
        artists.coverId,
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
    dynamic_cast<AlbumViewStyledDelegate*>(itemDelegate())->SetShowMode(AlbumViewStyledDelegate::SHOW_YEAR);
}

void AlbumView::FilterCategories(const QString & category) {
    last_query_ = qTEXT(R"(
SELECT
    albums.album,
    albums.coverId,
    artists.artist,
    albums.albumId,
    artists.artistId,
    artists.coverId,
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
    dynamic_cast<AlbumViewStyledDelegate*>(itemDelegate())->SetShowMode(AlbumViewStyledDelegate::SHOW_ARTIST);
}

void AlbumView::ShowAll() {
    last_query_ = qTEXT(R"(
SELECT
    albums.album,
    albums.coverId,
    artists.artist,
    albums.albumId,
    artists.artistId,
    artists.coverId,
    albums.year,
    albums.heart
FROM
    albums
LEFT 
	JOIN artists ON artists.artistId = albums.artistId
WHERE 
	albums.isPodcast = 0
ORDER BY
    albums.album DESC
    )");   
    dynamic_cast<AlbumViewStyledDelegate*>(itemDelegate())->SetShowMode(AlbumViewStyledDelegate::SHOW_ARTIST);
}

void AlbumView::OnSearchTextChanged(const QString& text) {
    last_query_ = qTEXT(R"(
SELECT
    albums.album,
    albums.coverId,
    artists.artist,
    albums.albumId,
    artists.artistId,
    artists.coverId,
    albums.year
FROM
    albums
    LEFT JOIN artists ON artists.artistId = albums.artistId
WHERE
    (
    albums.album LIKE '%%1%' OR artists.artist LIKE '%%1%'
    ) AND (albums.isPodcast = 0)
LIMIT 200
    )").arg(text);
    dynamic_cast<AlbumViewStyledDelegate*>(itemDelegate())->SetShowMode(AlbumViewStyledDelegate::SHOW_ARTIST);
}

void AlbumView::Update() {
    if (last_query_.isEmpty()) {
        ShowAll();
    }
    model_.setQuery(last_query_, qDatabase.database());
    if (model_.lastError().type() != QSqlError::NoError) {
        XAMP_LOG_DEBUG("SqlException: {}", model_.lastError().text().toStdString());
    }
}

void AlbumView::OnCurrentThemeChanged(ThemeColor theme_color) {
    page_->OnCurrentThemeChanged(theme_color);
}

void AlbumView::Refresh() {
    Update();
}

void AlbumView::append(const QString& file_name) {
    /*read_progress_dialog_ = MakeProgressDialog(kApplicationTitle,
        tr("Read track information"),
        tr("Cancel"));*/
    ReadSingleFileTrackInfo(file_name);
}

void AlbumView::ReadSingleFileTrackInfo(const QString& file_name) {
    const auto facade = QSharedPointer<DatabaseFacade>(new DatabaseFacade());

    (void)QObject::connect(facade.get(),
        &DatabaseFacade::ReadCompleted,
        this,
        &AlbumView::LoadCompleted);

    /*(void)QObject::connect(facade.get(),
        &DatabaseFacade::ReadFileStart,
        this,
        &AlbumView::OnReadFileStart);

    (void)QObject::connect(facade.get(),
        &DatabaseFacade::ReadFileProgress,
        this, 
        &AlbumView::OnReadFileProgress);

    (void)QObject::connect(facade.get(),
        &DatabaseFacade::ReadCurrentFilePath,
        this, 
        &AlbumView::OnReadCurrentFilePath);*/

    (void)QObject::connect(facade.get(),
        &DatabaseFacade::ReadFileEnd,
        this, 
        &AlbumView::OnReadFileEnd);

    facade->ReadTrackInfo(file_name, -1, false);
}

void AlbumView::OnReadFileStart(int dir_size) {
    if (!read_progress_dialog_) {
        return;
    }
    read_progress_dialog_->SetRange(0, dir_size);
}

void AlbumView::OnReadCurrentFilePath(const QString& dir, int32_t total_tracks, int32_t num_track) {
    if (!read_progress_dialog_) {
        return;
    }
    read_progress_dialog_->SetLabelText(dir);
    read_progress_dialog_->SetSubValue(total_tracks, num_track);
    qApp->processEvents();
}

void AlbumView::OnReadFileProgress(int progress) {
    if (!read_progress_dialog_) {
        return;
    }
    read_progress_dialog_->SetValue(progress);
}

void AlbumView::OnReadFileEnd() {
    if (!read_progress_dialog_) {
        return;
    }
    read_progress_dialog_.reset();
}

void AlbumView::HideWidget() {
    if (!page_) {
        return;
    }
    page_->hide();
}

void AlbumView::resizeEvent(QResizeEvent* event) {
    if (page_ != nullptr) {
        if (!page_->isHidden()) {
            const auto list_view_rect = this->rect();
            page_->setFixedSize(QSize(list_view_rect.size().width() - 15, list_view_rect.height() + 15));
            page_->move(QPoint(list_view_rect.x() + 3, 0));
        }
    }    
    QListView::resizeEvent(event);
}
