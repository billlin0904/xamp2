#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>

#include <widget/appsettings.h>
#include <widget/appsettingnames.h>
#include <widget/xmessagebox.h>
#include <widget/dao/playlistdao.h>
#include <widget/dao/musicdao.h>
#include <widget/xtooltip.h>
#include <widget/util/str_util.h>
#include <widget/playlisttabbar.h>
#include <widget/playlistpage.h>
#include <widget/playlisttableview.h>
#include <widget/actionmap.h>
#include <widget/playlisttabwidget.h>
#include <widget/imagecache.h>
#include <widget/util/image_util.h>

#include <xampplayer.h>
#include <widget/util/ui_util.h>

PlaylistTabWidget::PlaylistTabWidget(QWidget* parent)
    : QTabWidget(parent) {
    setObjectName("playlistTab");
    setMouseTracking(true);

    tooltip_timer_ = new QTimer(this);
    tooltip_timer_->setSingleShot(true);
    tooltip_timer_->setInterval(1000);
    
    tab_bar_ = new PlaylistTabBar(this);
    setTabBar(tab_bar_);

    (void)QObject::connect(tooltip_timer_, &QTimer::timeout, [this]() {
        if (hovered_tab_index_ != -1) {
            // Show the tooltip for the hovered tab index
            auto pos = tab_bar_->tabRect(hovered_tab_index_).center();
            toolTipMove(pos);
        }
		});

    add_tab_button_ = new QPushButton(this);    
    add_tab_button_->setMaximumSize(32, 32);
    add_tab_button_->setMinimumSize(32, 32);
    add_tab_button_->setIcon(qTheme.fontIcon(Glyphs::ICON_ADD));
#ifdef Q_OS_WIN
    add_tab_button_->setIconSize(QSize(18, 18));
#else
    add_tab_button_->setIconSize(QSize(18, 18));
#endif
    add_tab_button_->setObjectName("plusButton"_str);

    auto* button_widget = new QWidget(this);
    button_widget->setStyleSheet(qFormat(R"(background-color: transparent;)"));
    auto* layout = new QHBoxLayout(button_widget);
	layout->setContentsMargins(0, 0, 4, 4);
	layout->setSpacing(0);
    layout->addWidget(add_tab_button_, 1);

    setCornerWidget(button_widget);

    (void)QObject::connect(add_tab_button_, &QPushButton::pressed, [this]() {
        emit createNewPlaylist();
        });

    tabBar()->installEventFilter(this);

    setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(this, &PlaylistTabWidget::customContextMenuRequested, [this](auto pt) {
        ActionMap<PlaylistTabWidget> action_map(this);

        if (store_type_ == StoreType::CLOUD_STORE) {
            action_map.setCallback(action_map.addAction(tr("Create a playlist")), [this]() {
                emit createCloudPlaylist();
                });

            auto* reload_the_tab_act = action_map.addAction(tr("Reload all playlist"), [this]() {
                emit reloadAllPlaylist();
                });

            auto* reload_the_playlist_act = action_map.addAction(tr("Reload the playlist"), [pt, this]() {
                auto tab_index = tabBar()->tabAt(pt);
                if (tab_index == -1) {
                    return;
                }
                emit reloadPlaylist(tab_index);
                });

            auto* delete_the_tab_act = action_map.addAction(tr("Delete the playlist"), [pt, this]() {
                auto tab_index = tabBar()->tabAt(pt);
                if (tab_index == -1) {
                    return;
                }

                auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(tab_index));
                if (!playlist_page->playlist()->cloudPlaylistId()) {
                    return;
                }

                emit deletePlaylist(playlist_page->playlist()->cloudPlaylistId().value());
                });
        } else {
            auto* close_all_tab_act = action_map.addAction(tr("Close all tab"), [this]() {
                closeAllTab();
                });

            auto* close_other_tab_act = action_map.addAction(tr("Close other tab"), [pt, this]() {
                auto index = tabBar()->tabAt(pt);
                if (index == -1) {
                    return;
                }

                const auto* exclude_page = widget(index);

                QList<QWidget*> playlist_pages;
                for (auto i = 0; i < count(); ++i) {
                    auto* playlist_page = widget(i);
                    playlist_pages.append(playlist_page);
                }

                Q_FOREACH(auto * playlist_page, playlist_pages) {
                    if (playlist_page == exclude_page) {
                        continue;
                    }
                    const auto tab_index = indexOf(playlist_page);
                    closeTab(tab_index);
                }
                emit removeAllPlaylist();
                });
        }

        action_map.addSeparator();

        action_map.addAction(tr("Save playlist file"), [pt, this]() {
            auto tab_index = tabBar()->tabAt(pt);
            if (tab_index == -1) {
                return;
            }

            auto* playlist_page = playlistPage(tab_index);
            emit saveToM3UFile(playlist_page->playlist()->playlistId(), tabBar()->tabText(tab_index));
			});

        action_map.addAction(tr("Load playlist file"), [pt, this]() {
            auto tab_index = tabBar()->tabAt(pt);
            if (tab_index == -1) {
                return;
            }

            auto* playlist_page = playlistPage(tab_index);
            emit loadPlaylistFile(playlist_page->playlist()->playlistId());
        });

        XAMP_TRY_LOG(
            action_map.exec(pt);
        );
        });

    (void)QObject::connect(tab_bar_, &PlaylistTabBar::tabBarClicked, [this](auto index) {
        auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(index));
        Q_ASSERT(playlist_page != nullptr);
        playlist_page->playlist()->reload();
        });


    (void)QObject::connect(tab_bar_, &PlaylistTabBar::textChanged, [this](auto index, const auto& name) {
        playlist_dao_.setPlaylistName(currentPlaylistId(), name);
		qAppSettings.setValue(kAppSettingLastPlaylistTabIndex, currentPlaylistId());
        });

    (void)QObject::connect(tab_bar_, &PlaylistTabBar::tabBarClicked, [this](auto index) {
        auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(index));
        Q_ASSERT(playlist_page != nullptr);
        qAppSettings.setValue(kAppSettingLastPlaylistTabIndex, playlist_page->playlist()->playlistId());
        });

    (void)QObject::connect(this, &QTabWidget::tabCloseRequested,
        [this](auto tab_index) {
        /*if (XMessageBox::showYesOrNo(tr("Do you want to close tab ?")) == QDialogButtonBox::No) {
            return;
        }*/
        auto* playlist_page = playlistPage(tab_index);
        QString playlist;
        if (playlist_page->playlist()->cloudPlaylistId()) {
            playlist = playlist_page->playlist()->cloudPlaylistId().value();
        }
        emit deletePlaylist(playlist);
        closeTab(tab_index);
        if (!count()) {
            emit removeAllPlaylist();
        }
        });

	tooltip_ = new XTooltip();
    tooltip_->hide();
    onThemeChangedFinished(qTheme.themeColor());     
}

void PlaylistTabWidget::hidePlusButton() {
    add_tab_button_->hide();
}

void PlaylistTabWidget::onRetranslateUi() {

}

void PlaylistTabWidget::onThemeChangedFinished(ThemeColor theme_color) {
    switch (theme_color) {
    case ThemeColor::DARK_THEME:
        setStyleSheet(qFormat(R"(
    QTabBar { 
        background: #121212; 
    }

	QTabWidget#playlistTab {
		qproperty-iconSize: 16px 16px;
        border: none;
	}

    QTabWidget::pane {
        background-color: #121212;  
        border: 0px;      
    }

    QTabWidget::tab-bar {
        left: 0;    
        background-color: #121212;    
    }

	QTabWidget QTabBar::tab {
		min-height: 32px;
		background-color: #121212;        
	}

	QTabWidget QTabBar::tab:selected {		
		background-color: #54687A;
		font-weight: bold;
	}
    )"));

    add_tab_button_->setStyleSheet(qFormat(R"(
	QPushButton#plusButton {
        background-color: transparent;
    }

    QPushButton#plusButton:hover {
        background-color: #455364;
    }
    )"));
        break;
    case ThemeColor::LIGHT_THEME:
        setStyleSheet(qFormat(R"(	
	QTabWidget#playlistTab {
		qproperty-iconSize: 16px 16px;
        background-color: #f9f9f9;
	}

    QTabWidget::pane {        
        border: 0;
        background-color: #f9f9f9;
    }

    QTabWidget::tab-bar {
        left: 0;
    }

	QTabWidget QTabBar::tab {
		min-height: 32px;
		color: black;
		background-color: #f9f9f9;
	}

	QTabWidget QTabBar::tab:selected {		
		background-color: #C9CDD0;
		font-weight: bold;
	}
    )"));

    add_tab_button_->setStyleSheet(qFormat(R"(
	QPushButton#plusButton {
        background-color: transparent;
    }

    QPushButton#plusButton:hover {
        background-color: #e1e3e5;
    }
    )"));
        break;
    default:
        break;
    }
    add_tab_button_->setIcon(qTheme.fontIcon(Glyphs::ICON_ADD));

    for (auto i = 0; i < tabBar()->count(); ++i) {
        setTabIcon(i, qTheme.fontIcon(Glyphs::ICON_DRAFT));  
        auto* playlist_page = playlistPage(i);
        switch (qTheme.themeColor()) {
        case ThemeColor::DARK_THEME:
            playlist_page->setStyleSheet("QFrame#playlistPage { border: none; background-color: #121212; }"_str);
            break;
        case ThemeColor::LIGHT_THEME:
            playlist_page->setStyleSheet("QFrame#playlistPage { border: none; background-color: #f9f9f9; }"_str);
            break;
        }
    }
    if (tabBar()->count() > 0) {
        tabBar()->setTabButton(0, QTabBar::RightSide, nullptr);
    }
}

bool PlaylistTabWidget::removePlaylist(int32_t playlist_id) {
    Transaction scope;

    return scope.complete([&]() {
        playlist_dao_.removePlaylistAllMusic(playlist_id);
        playlist_dao_.removePlaylist(playlist_id);
        });
}

void PlaylistTabWidget::toolTipMove(const QPoint& pos) {
    if (hovered_tab_index_ == -1) {
        tooltip_->hide();
        return;
    }

    constexpr QSize kImageSize(150, 185);
    constexpr QSize kCoverSize(150, 150);

    auto index = hovered_tab_index_;
	auto* playlist_page = playlistPage(index);

    auto rect = tab_bar_->tabRect(index);
    auto global_pos = mapToGlobal(rect.center());
    global_pos.setY(global_pos.y() + 24);

    auto stats = playlist_dao_.getAlbumStats(playlist_page->playlist()->playlistId());

    auto tooltip_text = qFormat(tr("%1, %2 Tracks: %3"))
		.arg(stats.album_count)
		.arg(stats.music_count)
		.arg(formatDuration(stats.total_duration));

    tooltip_->setText(tooltip_text);
	tooltip_->setTextFont(QFont("FormatFont"_str, 10));
	tooltip_->setTextAlignment(Qt::AlignCenter);
    tooltip_->move(global_pos - QPoint(rect.size().width() / 4, 0));
    tooltip_->setImageSize(kImageSize);

    if (index == tab_bar_->currentIndex()) {
        tooltip_->setImage(QPixmap());
        tooltip_->hide();
        return;
    }

    auto cover_ids = playlist_dao_.getAlbumCoverIds(playlist_page->playlist()->playlistId());

    QList<QPixmap> images;
    Q_FOREACH(auto cover_id, cover_ids) {
        auto cover = qImageCache.getOrDefault(kAlbumCacheTag, cover_id);
        auto image = image_util::resizeImage(cover, kCoverSize);
        images.push_back(image);
    }

    if (!images.empty() && images.size() < 4) {
        auto image_size = images.size();
        for (auto i = 0; i < 4 - image_size; ++i) {
            images.push_back(image_util::resizeImage(qTheme.defaultSizeUnknownCover(), kImageSize));
            tooltip_->setImage(image_util::mergeImage(images));
        }
    } else {
        if (!images.empty()) {
            tooltip_->setImage(image_util::mergeImage(images));
        } else {
            tooltip_->setImage(qTheme.defaultSizeUnknownCover());
        }
    }
   
    tooltip_->showAndStart(false);
}

void PlaylistTabWidget::mouseMoveEvent(QMouseEvent* event) {
    auto pos = event->pos();
    auto index = tab_bar_->tabAt(pos);

    if (index != hovered_tab_index_) {
        // The hovered tab has changed
        hovered_tab_index_ = index;
        tooltip_timer_->stop();
        tooltip_->hide();

        if (hovered_tab_index_ != -1) {
            // Start the timer
            tooltip_timer_->start();
        }
    }
	QTabWidget::mouseMoveEvent(event);
}

void PlaylistTabWidget::mousePressEvent(QMouseEvent* event) {
    tooltip_timer_->stop();
    tooltip_->hide();
    QTabWidget::mousePressEvent(event);
}

void PlaylistTabWidget::leaveEvent(QEvent* event) {
	tooltip_->hide();
    hovered_tab_index_ = -1;
    tooltip_timer_->stop();
	QTabWidget::leaveEvent(event);
}

void PlaylistTabWidget::closeTab(int32_t tab_index) {
    auto* playlist_page = playlistPage(tab_index);
    const auto* playlist = playlist_page->playlist();
    --tab_count_;
    if (removePlaylist(playlist->playlistId())) {
        removeTab(tab_index);
        playlist_page->deleteLater();
    }
    resizeTabWidth();
}

void PlaylistTabWidget::createNewTab(const QString& name, QWidget* widget, bool resize) {
    switch (qTheme.themeColor()) {
    case ThemeColor::DARK_THEME:
        widget->setStyleSheet("QFrame#playlistPage { border: none; background-color: #121212; }"_str);
        break;
    case ThemeColor::LIGHT_THEME:
        widget->setStyleSheet("QFrame#playlistPage { border: none; background-color: #f9f9f9; }"_str);
        break;
    }

    const auto index = addTab(widget, name);
    ++tab_count_;
    tab_bar_->setTabCount(tab_count_);
    setTabIcon(index, qTheme.fontIcon(Glyphs::ICON_DRAFT));
    setCurrentIndex(index);
    if (resize) {
        resizeTabWidth();
    }
}

int32_t PlaylistTabWidget::currentPlaylistId() const {
    return dynamic_cast<PlaylistPage*>(currentWidget())->playlist()->playlistId();
}

void PlaylistTabWidget::setCurrentTabIndex(int32_t playlist_id) {
	for (auto i = 0; i < tabBar()->count(); ++i) {
		if (playlistPage(i)->playlist()->playlistId() == playlist_id) {
			setCurrentIndex(i);
			break;
		}
	}
}

void PlaylistTabWidget::saveTabOrder() const {
    for (auto i = 0; i < tabBar()->count(); ++i) {
        auto* playlist_page = playlistPage(i);
        const auto* playlist = playlist_page->playlist();
        playlist_dao_.setPlaylistIndex(playlist->playlistId(), i, store_type_);
        XAMP_LOG_DEBUG("saveTabOrder: {} at {}", tabText(i).toStdString(), i);
    }
}

void PlaylistTabWidget::restoreTabOrder() {
    const auto playlist_index = playlist_dao_.getPlaylistIndex(store_type_);

    QList<QString> texts;
    QList<int> new_order;

    for (auto& [index, playlist_id] : playlist_index) {
        auto found = false;
        for (auto j = 0; j < tabBar()->count(); ++j) {
            auto* playlist_page = playlistPage(j);
			playlist_page->playlist()->reload();
            const auto widget_playlist_id = playlist_page->playlist()->playlistId();
            if (widget_playlist_id == playlist_id) {
                XAMP_LOG_DEBUG("restoreTabOrder: {} at {}", widget_playlist_id, j);
                texts.append(tabText(j));
                new_order.append(j);
                found = true;
                break;
            }
        }

        Q_ASSERT(found);
    }

    // 移動 tab 的順序
    for (auto i = 0; i < new_order.size(); ++i) {
        tabBar()->moveTab(new_order[i], i);
    }

    // 重新設置 tab 的標籤
    for (auto i = 0; i < new_order.size(); ++i) {
        setTabText(i, texts[i]);
    }

    // 重新設置 tab 的 icon
    for (auto i = 0; i < new_order.size(); ++i) {
        setTabIcon(i, qTheme.fontIcon(Glyphs::ICON_DRAFT));
    }

    for (auto i = 0; i < tabBar()->count(); ++i) {
        XAMP_LOG_DEBUG("restoreTabOrder: {} at {}", tabText(i).toStdString(), i);
    }
}

void PlaylistTabWidget::resetAllTabIcon() {
    for (auto i = 0; i < tabBar()->count(); ++i) {
        setTabIcon(i, qTheme.fontIcon(Glyphs::ICON_DRAFT));
    }
}

void PlaylistTabWidget::setPlaylistTabIcon(const QIcon& icon) {
	const auto tab_index = currentIndex();
    if (tab_index != -1) {
        setTabIcon(tab_index, icon);
        for (auto i = 0; i < tabBar()->count(); ++i) {
            if (i == tab_index) {
                continue;
            }
            setTabIcon(i, qTheme.fontIcon(Glyphs::ICON_DRAFT));
        }
    }
}

void PlaylistTabWidget::setPlaylistCover(const QPixmap& cover) {
    for (auto i = 0; i < tabBar()->count(); ++i) {
        playlistPage(i)->setCover(&cover);
    }
}

void PlaylistTabWidget::mouseDoubleClickEvent(QMouseEvent*) {
    emit createNewPlaylist();
}

void PlaylistTabWidget::resizeTabWidth() {
    auto w = width();
    if (tab_bar_->count() <= 3) {
		tab_bar_->setMinimumWidth(100);
		return;
    }
    tab_bar_->setMinimumWidth(w - 45);
	/*constexpr auto kMaxTabWidth = 300;
    auto max_width = getMainWindow()->width() - 150;
    tab_bar_->setMaximumWidth(max_width);
    auto average_width = tab_bar_->count() > (max_width / kMaxTabWidth)
		? max_width / tab_bar_->count() : kMaxTabWidth;
    setStyleSheet(qFormat(R"(		
	    QTabWidget QTabBar::tab {
            min-height: 32px;
			width: %1px; 
        }
	)").arg(average_width));*/
}

void PlaylistTabWidget::resizeEvent(QResizeEvent* event) {
    QTabWidget::resizeEvent(event);
    resizeTabWidth();
}

bool PlaylistTabWidget::eventFilter(QObject* watched, QEvent* event) {
    return QTabWidget::eventFilter(watched, event);
}

void PlaylistTabWidget::closeAllTab() {
    QList<PlaylistPage*> playlist_pages;
    for (auto i = 0; i < tabBar()->count(); ++i) {
        playlist_pages.append(playlistPage(i));
    }

    Q_FOREACH(auto * page, playlist_pages) {
        removePlaylist(page->playlist()->playlistId());
        page->deleteLater();
    }

    playlist_dao_.forEachPlaylist([this](auto playlist_id,
        auto,
        auto store_type,
        auto,
        auto) {
            if (playlist_id == kAlbumPlaylistId
                || playlist_id == kCdPlaylistId
                || playlist_id == kYtMusicSearchPlaylistId) {
                return;
            }
            if (store_type != store_type_) {
                return;
            }
            removePlaylist(playlist_id);
        });

    tab_count_ = 0;
    tab_bar_->setTabCount(tab_count_);
    clear();
    emit removeAllPlaylist();
    resizeTabWidth();
}

void PlaylistTabWidget::setStoreType(StoreType type) {
    store_type_ = type;
    setTabsClosable(true);
    setMovable(store_type_ == StoreType::CLOUD_SEARCH_STORE ? true : false);
}

void PlaylistTabWidget::reloadAll() {
    for (auto i = 0; i < count(); ++i) {
        playlistPage(i)->playlist()->reload();
    }
}

PlaylistPage* PlaylistTabWidget::findPlaylistPage(int32_t playlist_id) {
    for (auto i = 0; i < count(); ++i) {
		auto* playlist_page = playlistPage(i);
        if (playlist_page->playlist()->playlistId() == playlist_id) {
            return playlist_page;
        }
    }
    return nullptr;
}

PlaylistPage* PlaylistTabWidget::playlistPage(int32_t index) const {
    auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(index));
    XAMP_ENSURES(playlist_page != nullptr);
	return playlist_page;
}

void PlaylistTabWidget::setCurrentNowPlaying() {
    setPlaylistTabIcon(qTheme.playlistPlayingIcon(PlaylistTabWidget::kTabIconSize, 1.4));
}

void PlaylistTabWidget::setNowPlaying(int32_t playlist_id) {
    auto icon = qTheme.playlistPlayingIcon(PlaylistTabWidget::kTabIconSize, 1.4);

    for (auto i = 0; i < count(); ++i) {
        if (playlistPage(i)->playlist()->playlistId() == playlist_id) {
            setTabIcon(i, icon);
        }
        else {
            setTabIcon(i, qTheme.fontIcon(Glyphs::ICON_DRAFT));
        }
    }
}

void PlaylistTabWidget::setPlayerStateIcon(int32_t playlist_id, PlayerState state) {
    QIcon icon;

    switch (state) {
    case PlayerState::PLAYER_STATE_RUNNING:
        icon = qTheme.playlistPlayingIcon(PlaylistTabWidget::kTabIconSize, 1.0);
        break;
    case PlayerState::PLAYER_STATE_PAUSED:
        icon = qTheme.playlistPauseIcon(PlaylistTabWidget::kTabIconSize, 1.0);
        break;
    case PlayerState::PLAYER_STATE_STOPPED:
    case PlayerState::PLAYER_STATE_USER_STOPPED:
    default:
        icon = qTheme.fontIcon(Glyphs::ICON_DRAFT);
        break;
    }

    for (auto i = 0; i < count(); ++i) {
        if (playlistPage(i)->playlist()->playlistId() == playlist_id) {
            setTabIcon(i, icon);
        }
        else {
            setTabIcon(i, qTheme.fontIcon(Glyphs::ICON_DRAFT));
        }
    }
}
