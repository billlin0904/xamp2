#include <QDebug>
#include <QToolTip>
#include <QFontDatabase>
#include <QMenu>
#include <QCloseEvent>
#include <QFileInfo>
#include <QDesktopWidget>
#include <QInputDialog>
#include <QShortcut>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QProgressDialog>

#include <player/chromaprinthelper.h>
#include <player/soxresampler.h>

#include <widget/albumartistpage.h>
#include <widget/lrcpage.h>
#include <widget/albumview.h>
#include <widget/artistview.h>
#include <widget/str_utilts.h>
#include <widget/playlistpage.h>
#include <widget/toast.h>
#include <widget/image_utiltis.h>
#include <widget/time_utilts.h>
#include <widget/database.h>
#include <widget/pixmapcache.h>
#include <widget/selectcolorwidget.h>
#include <widget/artistinfopage.h>
#include <widget/spectrograph.h>

#include "aboutdialog.h"
#include "preferencedialog.h"
#include "thememanager.h"
#include "xamp.h"

static MusicEntity toMusicEntity(const PlayListEntity& item) {
    MusicEntity music_entity;
    music_entity.music_id = item.music_id;
    music_entity.artist_id = item.artist_id;
    music_entity.album_id = item.album_id;
    music_entity.cover_id = item.cover_id;
    music_entity.title = item.title;
    music_entity.album = item.album;
    music_entity.artist = item.artist;
    music_entity.file_ext = item.file_ext;
    music_entity.file_path = item.file_path;
    return music_entity;
}

static QString getPlayEntityFormat(const AudioPlayer* player, const QString& file_ext) {
    auto format = player->GetStreamFormat();
    auto dsd_speed = player->GetDSDSpeed();

    auto ext = file_ext;
    ext = ext.remove(Q_UTF8(".")).toUpper();
    auto is_mhz_samplerate = false;
    auto precision = 1;
    if (format.GetSampleRate() / 1000 > 1000) {
        is_mhz_samplerate = true;
    }
    else {
        precision = format.GetSampleRate() % 1000 == 0 ? 0 : 1;
    }
    auto bits = format.GetBitsPerSample();
    if (is_mhz_samplerate) {
        bits = 1;
    }

    QString dsd_speed_format;
    if (dsd_speed) {
        dsd_speed_format = Q_UTF8(" DSD") + QString::number(dsd_speed.value()) + Q_UTF8(" ");
	}

    QString dsd_mode;
    switch (player->GetDSDModes()) {
    case DsdModes::DSD_MODE_PCM:
        dsd_mode = Q_UTF8("PCM");
        break;
    case DsdModes::DSD_MODE_NATIVE:
        dsd_mode = Q_UTF8("Native DSD");
        break;
    case DsdModes::DSD_MODE_DOP:
        dsd_mode = Q_UTF8("DOP");
        break;
    }

    return ext
        + Q_UTF8(" | ")
        + dsd_speed_format
        + (is_mhz_samplerate ? QString::number(format.GetSampleRate() / double(1000000), 'f', 2) + Q_UTF8("MHz/")
            : QString::number(format.GetSampleRate() / double(1000), 'f', precision) + Q_UTF8("kHz/"))
        + QString::number(bits) + Q_UTF8("bit")
        + Q_UTF8(" | ") + dsd_mode;
}

Xamp::Xamp(QWidget *parent)
    : FramelessWindow(parent)
    , is_seeking_(false)
    , order_(PlayerOrder::PLAYER_ORDER_REPEAT_ONCE)
    , lrc_page_(nullptr)
    , playlist_page_(nullptr)
    , album_artist_page_(nullptr)
    , artist_info_page_(nullptr)
    , playback_history_page_(nullptr)
    , state_adapter_(std::make_shared<PlayerStateAdapter>())
    , player_(std::make_shared<AudioPlayer>(state_adapter_)) {
}

void Xamp::initial() {
    initialUI();
    initialController();
    initialDeviceList();
    initialPlaylist();
    setCover(ThemeManager::instance().pixmap().unknownCover());
    DeviceFactory::Instance().RegisterDeviceListener(player_);
    setDefaultStyle();
}

void Xamp::closeEvent(QCloseEvent*) {
	AppSettings::setValue(APP_SETTING_VOLUME, player_->GetVolume());
    AppSettings::setValue(APP_SETTING_WIDTH, size().width());
    AppSettings::setValue(APP_SETTING_HEIGHT, size().height());
	AppSettings::setValue(APP_SETTING_VOLUME, ui.volumeSlider->value());

    if (player_ != nullptr) {
        player_->Destory();
        player_.reset();
    }
}

void Xamp::setNightStyle() {
}

void Xamp::setDefaultStyle() {
    ThemeManager::instance().setPlayOrPauseButton(ui, false);
    ThemeManager::instance().setDefaultStyle(ui);
    applyTheme(ThemeManager::instance().getBackgroundColor());
    setStyleSheet(Q_UTF8(R"(
                         QTableView {
                         background-color: transparent;
                         color: black;
                         }

                         QFrame#playingFrame {
                         background-color: transparent;
                         border: none;
                         }
                         )"));
}

void Xamp::registerMetaType() {
    qRegisterMetaType<std::vector<xamp::base::Metadata>>("std::vector<xamp::base::Metadata>");
    qRegisterMetaType<std::vector<float>>("std::vector<float>");
    qRegisterMetaType<xamp::output_device::DeviceState>("xamp::output_device::DeviceState");
    qRegisterMetaType<xamp::player::PlayerState>("xamp::player::PlayerState");
    qRegisterMetaType<xamp::base::Errors>("xamp::base::Errors");
}

void Xamp::initialUI() {
    registerMetaType();

    ui.setupUi(this);

    watch_.addPath(AppSettings::getValue(APP_SETTING_MUSIC_FILE_PATH).toString());

    setGeometry(QStyle::alignedRect(Qt::LeftToRight,
                                    Qt::AlignCenter,
                                    AppSettings::getSizeValue(APP_SETTING_WIDTH, APP_SETTING_HEIGHT),
                                    qApp->desktop()->availableGeometry()));

	auto f = font();
	f.setPointSize(10);
	ui.titleLabel->setFont(f);
	f.setPointSize(8);
	ui.artistLabel->setFont(f);

#ifdef Q_OS_WIN
    f.setPointSize(7);
    ui.startPosLabel->setFont(f);
    ui.endPosLabel->setFont(f);
#endif

#ifdef Q_OS_MAC
    ui.closeButton->hide();
    ui.maxWinButton->hide();
    ui.minWinButton->hide();
    f.setPointSize(11);
    ui.titleLabel->setFont(f);
    f.setPointSize(10);
    ui.artistLabel->setFont(f);
#endif

    if (AppSettings::getValueAsString(APP_SETTING_LANG).isEmpty()) {
        LocaleLanguage l;
        XAMP_LOG_DEBUG("Load locale lang file: {}", l.getIsoCode().toStdString());
        lang_manager_.loadLanguage(l.getIsoCode());
        AppSettings::setValue(APP_SETTING_LANG, l.getIsoCode());
    }
    else {
        lang_manager_.loadLanguage(AppSettings::getValueAsString(APP_SETTING_LANG));
    }
}

QWidgetAction* Xamp::createTextSeparator(const QString& text) {
    auto label = new QLabel(text);
    label->setObjectName(Q_UTF8("textSeparator"));
    auto f = font();
    f.setBold(true);
    label->setFont(f);
    auto separator = new QWidgetAction(this);
    separator->setDefaultWidget(label);
    return separator;
}

void Xamp::onDeviceStateChanged(DeviceState state) {
    if (state == DeviceState::DEVICE_STATE_REMOVED) {
        player_->Stop(true, true, true);
    }
    if (state == DeviceState::DEVICE_STATE_DEFAULT_DEVICE_CHANGE) {
        return;
    }
    initialDeviceList();
}

void Xamp::initialDeviceList() {    
    auto menu = ui.selectDeviceButton->menu();
    if (!menu) {
        menu = new QMenu(this);
        ui.selectDeviceButton->setMenu(menu);
    }

    menu->setStyleSheet(ThemeManager::instance().getMenuStyle());
    menu->clear();

    DeviceInfo init_device_info;
    auto is_find_setting_device = false;

    auto device_action_group = new QActionGroup(this);
    device_action_group->setExclusive(true);

    std::map<std::wstring, QAction*> device_id_action;

    DeviceFactory::Instance().ForEach([&](const auto &device_type) {
        device_type->ScanNewDevice();

        const auto device_info_list = device_type->GetDeviceInfo();
        if (device_info_list.empty()) {
            return;
        }

		auto desc = device_type->GetDescription();
		menu->addAction(createTextSeparator(QLatin1String{ desc.data(), (int)desc.length() }));

        for (const auto& device_info : device_info_list) {
            auto device_action = new QAction(QString::fromStdWString(device_info.name), this);
            device_action_group->addAction(device_action);
            device_action->setCheckable(true);
            device_id_action[device_info.device_id] = device_action;
            (void)QObject::connect(device_action, &QAction::triggered, [device_info, this]() {
                device_info_ = device_info;
                AppSettings::setValue(APP_SETTING_DEVICE_TYPE, device_info_.device_type_id);
                AppSettings::setValue(APP_SETTING_DEVICE_ID, device_info_.device_id);
            });
            menu->addAction(device_action);
            if (AppSettings::getID(APP_SETTING_DEVICE_TYPE) == device_info.device_type_id
                    && AppSettings::getValue(APP_SETTING_DEVICE_ID).toString().toStdWString() == device_info.device_id) {
                device_info_ = device_info;
                is_find_setting_device = true;
                device_action->setChecked(true);
            }
        }

        if (!is_find_setting_device) {
            auto itr = std::find_if(device_info_list.begin(), device_info_list.end(), [](const auto& info) {
                return info.is_default_device && !DeviceFactory::IsExclusiveDevice(info);
            });
            if (itr != device_info_list.end()) {
                init_device_info = (*itr);
            }
        }
    });

    if (!is_find_setting_device) {
        device_info_ = init_device_info;
        device_id_action[device_info_.device_id]->setChecked(true);
        AppSettings::setValue(APP_SETTING_DEVICE_TYPE, device_info_.device_type_id);
        AppSettings::setValue(APP_SETTING_DEVICE_ID, device_info_.device_id);
    }
}

void Xamp::initialController() {
    (void)QObject::connect(ui.minWinButton, &QToolButton::pressed, [this]() {
        showMinimized();
    });

    (void)QObject::connect(ui.maxWinButton, &QToolButton::pressed, [this]() {
        if (isMaximized()) {
            showNormal();
        }
        else {
            showMaximized();
        }
    });

    (void)QObject::connect(ui.mutedButton, &QToolButton::pressed, [this]() {
        if (player_->IsMute()) {
            player_->SetMute(false);            
            ui.mutedButton->setIcon(ThemeManager::instance().volumeUp());
        } else {
            player_->SetMute(true);
            ui.mutedButton->setIcon(ThemeManager::instance().volumeOff());
        }
    });

    (void)QObject::connect(ui.closeButton, &QToolButton::pressed, [this]() {
        QWidget::close();
    });

    (void)QObject::connect(ui.settingsButton, &QToolButton::pressed, []() {

    });

	(void)QObject::connect(ui.seekSlider, &SeekSlider::leftButtonValueChanged, [this](auto value) {
		try {
			player_->Seek(static_cast<double>(value / 1000.0));
		}
		catch (const xamp::base::Exception & e) {
			player_->Stop(false);
			Toast::showTip(Q_UTF8(e.GetErrorMessage()), this);
			return;
		}
		});

    (void)QObject::connect(ui.seekSlider, &SeekSlider::sliderMoved, [this](auto value) {
        QToolTip::showText(QCursor::pos(), Time::msToString(double(ui.seekSlider->value()) / 1000.0));
        if (!is_seeking_) {
            return;
        }
        ui.seekSlider->setValue(value);
    });

    (void)QObject::connect(ui.seekSlider, &SeekSlider::sliderReleased, [this]() {
        QToolTip::showText(QCursor::pos(), Time::msToString(double(ui.seekSlider->value()) / 1000.0));
        if (!is_seeking_) {
            return;
        }
        try {
            player_->Seek(static_cast<double>(ui.seekSlider->value() / 1000.0));
        }
        catch (const xamp::base::Exception& e) {
            player_->Stop(false);
            Toast::showTip(Q_UTF8(e.GetErrorMessage()), this);
            return;
        }
        is_seeking_ = false;
    });

    (void)QObject::connect(ui.seekSlider, &SeekSlider::sliderPressed, [this]() {
        QToolTip::showText(QCursor::pos(), Time::msToString(double(ui.seekSlider->value()) / 1000.0));
        if (is_seeking_) {
            return;
        }
        is_seeking_ = true;
        player_->Pause();
    });

    order_ = static_cast<PlayerOrder>(AppSettings::getAsInt(APP_SETTING_ORDER));
    setPlayerOrder();

	auto vol = AppSettings::getAsInt(APP_SETTING_VOLUME);
    ui.volumeSlider->setRange(0, 100);
    ui.volumeSlider->setValue(vol);
	player_->SetMute(vol == 0);
    player_->SetVolume(vol);

    (void)QObject::connect(ui.volumeSlider, &QSlider::valueChanged, [this](auto volume) {
        QToolTip::showText(QCursor::pos(), tr("Volume : ") + QString::number(volume) + Q_UTF8("%"));
        setVolume(volume);
    });

    (void)QObject::connect(ui.volumeSlider, &SeekSlider::leftButtonValueChanged, [this](auto volume) {
        QToolTip::showText(QCursor::pos(), tr("Volume : ") + QString::number(volume) + Q_UTF8("%"));
        setVolume(volume);
    });

    (void)QObject::connect(ui.volumeSlider, &QSlider::sliderMoved, [](auto volume) {
        QToolTip::showText(QCursor::pos(), tr("Volume : ") + QString::number(volume) + Q_UTF8("%"));
    });

    (void)QObject::connect(state_adapter_.get(),
                           &PlayerStateAdapter::stateChanged,
                           this,
                           &Xamp::onPlayerStateChanged,
                           Qt::QueuedConnection);

    (void)QObject::connect(state_adapter_.get(),
                           &PlayerStateAdapter::sampleTimeChanged,
                           this,
                           &Xamp::onSampleTimeChanged,
                           Qt::QueuedConnection);    

    (void)QObject::connect(state_adapter_.get(),
        &PlayerStateAdapter::deviceChanged,
        this,
        &Xamp::onDeviceStateChanged,
        Qt::QueuedConnection);

    (void)QObject::connect(ui.searchLineEdit, &QLineEdit::textChanged, [this](const auto &text) {
        if (ui.currentView->count() > 0) {
            auto playlist_view = playlist_page_->playlist();
            emit playlist_view->search(text, Qt::CaseSensitivity::CaseInsensitive, QRegExp::PatternSyntax());
            emit album_artist_page_->album()->onSearchTextChanged(text);
        }
    });

    (void)QObject::connect(ui.nextButton, &QToolButton::pressed, [this]() {
        playNextClicked();
    });

    (void)QObject::connect(ui.prevButton, &QToolButton::pressed, [this]() {
        playPreviousClicked();
    });

    (void)QObject::connect(ui.repeatButton, &QToolButton::pressed, [this]() {
        order_ = static_cast<PlayerOrder>((order_ + 1) % PlayerOrder::_MAX_PLAYER_ORDER_);
        setPlayerOrder();
    });

    (void)QObject::connect(ui.playButton, &QToolButton::pressed, [this]() {
        play();
    });

    (void)QObject::connect(ui.stopButton, &QToolButton::pressed, [this]() {
        stopPlayedClicked();
        });

    (void)QObject::connect(ui.backPageButton, &QToolButton::pressed, [this]() {
        goBackPage();
        album_artist_page_->refreshOnece();
    });

    (void)QObject::connect(ui.nextPageButton, &QToolButton::pressed, [this]() {
        getNextPage();
        album_artist_page_->refreshOnece();
        emit album_artist_page_->album()->onSearchTextChanged(ui.searchLineEdit->text());
    });

    (void)QObject::connect(ui.addPlaylistButton, &QToolButton::pressed, [this]() {
        auto pos = mapFromGlobal(QCursor::pos());
        playback_history_page_->move(QPoint(pos.x() - 300, pos.y() - 410));
        playback_history_page_->setMinimumSize(QSize(600, 400));
        playback_history_page_->refreshOnece();
        playback_history_page_->show();
    });

    (void)QObject::connect(ui.artistLabel, &ClickableLabel::clicked, [this]() {
        onArtistIdChanged(current_entiry_.artist, current_entiry_.cover_id, current_entiry_.artist_id);
    });

    (void)QObject::connect(ui.coverLabel, &ClickableLabel::clicked, [this]() {
        ui.currentView->setCurrentWidget(lrc_page_);
        });

    (void)QObject::connect(ui.sliderBar, &TabListView::clickedTable, [this](auto table_id) {
        setTablePlaylistView(table_id);
    });

    (void)QObject::connect(ui.sliderBar, &TabListView::tableNameChanged, [](auto table_id, const auto &name) {
        Database::instance().setTableName(table_id, name);
    });

    auto settings_menu = new QMenu(this);
    settings_menu->setStyleSheet(ThemeManager::instance().getMenuStyle());
    auto settings_action = new QAction(tr("Settings"), this);
    settings_menu->addAction(settings_action);
    (void)QObject::connect(settings_action, &QAction::triggered, [=]() {
        PreferenceDialog dialog;
        auto f = font();
#ifdef Q_OS_WIN
        f.setPointSize(8);
#endif
        dialog.setFont(f);
        dialog.exec();
        watch_.addPath(dialog.musicFilePath);
        });
    auto enable_blur_material_mode_action = new QAction(tr("Enable blur"), this);
    enable_blur_material_mode_action->setCheckable(true);
    if (AppSettings::getValue(APP_SETTING_ENABLE_BLUR).toBool()) {
        enable_blur_material_mode_action->setChecked(true);
    }
    (void)QObject::connect(enable_blur_material_mode_action, &QAction::triggered, [=]() {
        auto enable = AppSettings::getValueAsBool(APP_SETTING_ENABLE_BLUR);
        enable = !enable;
        enable_blur_material_mode_action->setChecked(enable);
        ThemeManager::instance().enableBlur(this, enable);
        });
    auto select_color_widget = new SelectColorWidget(this);
    auto theme_color_menu = new QMenu(tr("Theme color"));
    auto widget_action = new QWidgetAction(theme_color_menu);
    widget_action->setDefaultWidget(select_color_widget);
    (void)QObject::connect(select_color_widget, &SelectColorWidget::colorButtonClicked, [this](auto color) {        
        applyTheme(color);
        });
    theme_color_menu->addAction(widget_action);
    settings_menu->addMenu(theme_color_menu);
    settings_menu->addAction(enable_blur_material_mode_action);
    settings_menu->addSeparator();
    auto about_action = new QAction(tr("About"), this);
    settings_menu->addAction(about_action);
    (void)QObject::connect(about_action, &QAction::triggered, [=]() {
        AboutDialog aboutdlg;
        aboutdlg.setFont(font());
        aboutdlg.exec();
        });
    ui.settingsButton->setMenu(settings_menu);

    ui.seekSlider->setEnabled(false);
    ui.startPosLabel->setText(Time::msToString(0));
    ui.endPosLabel->setText(Time::msToString(0));
    ui.searchLineEdit->setPlaceholderText(tr("Search anything"));
}

void Xamp::applyTheme(QColor color) {
    if (qGray(color.rgb()) > 150) {
        playlist_page_->setTextColor(Qt::black);
        lrc_page_->setTextColor(Qt::black);
        emit textColorChanged(color, Qt::black);
        ThemeManager::instance().setThemeColor(ThemeColor::WHITE_THEME);        
    }
    else {
        playlist_page_->setTextColor(Qt::white);
        lrc_page_->setTextColor(Qt::white);
        emit textColorChanged(color, Qt::white);
        ThemeManager::instance().setThemeColor(ThemeColor::DARK_THEME);
    }    
    if (player_->GetState() == xamp::player::PlayerState::PLAYER_STATE_PAUSED) {
        ThemeManager::instance().setPlayOrPauseButton(ui, true);
    }
    else {
        ThemeManager::instance().setPlayOrPauseButton(ui, false);
    }
    ThemeManager::instance().setBackgroundColor(ui, color);
}

void Xamp::getNextPage() {
    if (stack_page_id_.isEmpty()) {
        return;
    }
    auto idx = ui.currentView->currentIndex();
    ui.currentView->setCurrentIndex(idx + 1);
}

void Xamp::setTablePlaylistView(int table_id) {
    auto playlist_id = Database::instance().findTablePlaylistId(table_id);

    bool found = false;
    for (auto idx : stack_page_id_) {
        if (auto page = dynamic_cast<PlyalistPage*>(ui.currentView->widget(idx))) {
            if (page->playlist()->playlistId() == playlist_id) {
                ui.currentView->setCurrentIndex(idx);
                found = true;
                break;
            }
        }
    }

    if (!found) {
        auto playlist_page = newPlaylist(playlist_id);
        playlist_page->playlist()->setPlaylistId(playlist_id);
        Database::instance().forEachPlaylistMusic(playlist_id, [this](const auto& entityy) {
            playlist_page_->playlist()->appendItem(entityy);
        });
        pushWidget(playlist_page);
    }
}

void Xamp::goBackPage() {
    if (stack_page_id_.isEmpty()) {
        return;
    }
    auto idx = ui.currentView->currentIndex();
    ui.currentView->setCurrentIndex(idx - 1);
}

void Xamp::setVolume(int32_t volume) {
    if (volume > 0) {
        player_->SetMute(false);
        ui.mutedButton->setIcon(ThemeManager::instance().volumeUp());
    }
    else {
        player_->SetMute(true);
        ui.mutedButton->setIcon(ThemeManager::instance().volumeOff());
    }

    try {
        player_->SetVolume(volume);
    }
    catch (const xamp::base::Exception& e) {
        player_->Stop(false);
        Toast::showTip(Q_UTF8(e.GetErrorMessage()), this);
    }
}

void Xamp::initialShortcut() {

}

void Xamp::stopPlayedClicked() {
    player_->Stop(false, true);
    setSeekPosValue(0);
    ui.seekSlider->setEnabled(false);
    playlist_page_->playlist()->removePlaying();
}

void Xamp::playNextClicked() {
    playNextItem(1);
}

void Xamp::playPreviousClicked() {
    playNextItem(-1);
}

void Xamp::deleteKeyPress() {
    if (!ui.currentView->count()) {
        return;
    }
    auto playlist_view = playlist_page_->playlist();
    playlist_view->removeSelectItems();
}

void Xamp::setPlayerOrder() {
    switch (order_) {
    case PlayerOrder::PLAYER_ORDER_REPEAT_ONCE:
        ThemeManager::instance().setRepeatOncePlayorder(ui);
        AppSettings::setValue(APP_SETTING_ORDER,
                                         (int)PLAYER_ORDER_REPEAT_ONCE);
        break;
    case PlayerOrder::PLAYER_ORDER_REPEAT_ONE:
        ThemeManager::instance().setRepeatOnePlayorder(ui);
        AppSettings::setValue(APP_SETTING_ORDER,
                                         (int)PLAYER_ORDER_REPEAT_ONE);
        break;
    case PlayerOrder::PLAYER_ORDER_SHUFFLE_ALL:
        ThemeManager::instance().setShufflePlayorder(ui);
        AppSettings::setValue(APP_SETTING_ORDER,
                                         (int)PlayerOrder::PLAYER_ORDER_SHUFFLE_ALL);
        break;
    default:
        break;
    }
}

void Xamp::playNextItem(int32_t forward) {
    auto playlist_view = playlist_page_->playlist();
    const auto count = playlist_view->model()->rowCount();
    if (count == 0) {
        stopPlayedClicked();
        return;
    }

    switch (order_) {
    case PlayerOrder::PLAYER_ORDER_REPEAT_ONE:
        play_index_ = playlist_view->currentIndex();
        break;
    case PlayerOrder::PLAYER_ORDER_REPEAT_ONCE:
        play_index_ = playlist_view->currentIndex();
        play_index_ = playlist_view->model()->index(play_index_.row() + forward, PLAYLIST_PLAYING);
        if (play_index_.row() == -1) {
            return;
        }
        break;
    case PlayerOrder::PLAYER_ORDER_SHUFFLE_ALL:
        if (count > 1) {
            play_index_ = playlist_view->shuffeIndex();
        }
        break;
    default:
        break;
    }

    if (!play_index_.isValid()) {
        Toast::showTip(tr("Not found any playlist item."), this);
        return;
    }
    playlist_view->setNowPlaying(play_index_, true);
    playlist_view->play(play_index_);
}

void Xamp::onSampleTimeChanged(double stream_time) {
    if (!player_) {
        return;
    }
    if (player_->GetState() == PlayerState::PLAYER_STATE_RUNNING) {         
        setSeekPosValue(stream_time);
    }
}

void Xamp::setSeekPosValue(double stream_time) {
    ui.endPosLabel->setText(Time::msToString(player_->GetDuration() - stream_time));
    auto stream_time_as_ms = static_cast<int32_t>(stream_time * 1000.0);
    ui.seekSlider->setValue(stream_time_as_ms);
    ui.startPosLabel->setText(Time::msToString(stream_time));
    setTaskbarProgress(100.0 * ui.seekSlider->value() / ui.seekSlider->maximum());
    lrc_page_->lyricsWidget()->setLrcTime(stream_time_as_ms);
}

void Xamp::playLocalFile(const PlayListEntity& item) {
    setTaskbarPlayerPlaying();
    play(item);
}

void Xamp::play() {
    if (player_->GetState() == xamp::player::PlayerState::PLAYER_STATE_RUNNING) {
        ThemeManager::instance().setPlayOrPauseButton(ui, false);
        player_->Pause();
        setTaskbarPlayerPaused();
    }
    else if (player_->GetState() == xamp::player::PlayerState::PLAYER_STATE_PAUSED) {
        ThemeManager::instance().setPlayOrPauseButton(ui, true);
        player_->Resume();
        setTaskbarPlayingResume();
    }
    else if (player_->GetState() == xamp::player::PlayerState::PLAYER_STATE_STOPPED) {
        if (!ui.currentView->count()) {
            return;
        }
        if (auto select_item = playlist_page_->playlist()->selectItem()) {
            play_index_ = select_item.value();
        }
        play_index_ = playlist_page_->playlist()->model()->index(
                    play_index_.row(), PLAYLIST_PLAYING);
        if (play_index_.row() == -1) {
            Toast::showTip(tr("Not found any playing item."), this);
            return;
        }
        playlist_page_->playlist()->setNowPlaying(play_index_, true);
        playlist_page_->playlist()->play(play_index_);
    }
}

void Xamp::resetSeekPosValue() {
    ui.seekSlider->setValue(0);
    ui.startPosLabel->setText(Time::msToString(0));
}

void Xamp::setupResampler() {
    if (AppSettings::getValue(APP_SETTING_RESAMPLER_ENABLE).toBool()) {
        auto samplerate = AppSettings::getValue(APP_SETTING_SOXR_RESAMPLE_SAMPLRATE).toUInt();
        auto quality = static_cast<SoxrQuality>(AppSettings::getValue(APP_SETTING_SOXR_QUALITY).toInt());
        auto phase = static_cast<SoxrPhaseResponse>(AppSettings::getValue(APP_SETTING_SOXR_PHASE).toInt());
        auto passband = AppSettings::getValue(APP_SETTING_SOXR_PASS_BAND).toInt();
        auto enable_steep_filter = AppSettings::getValue(APP_SETTING_SOXR_ENABLE_STEEP_FILTER).toBool();

        auto resampler = MakeAlign<Resampler, SoxrResampler>();
        auto soxr = dynamic_cast<SoxrResampler*>(resampler.get());
        soxr->SetQuality(quality);
        soxr->SetPhase(phase);
        soxr->SetPassBand(passband / 100.0);
        soxr->SetSteepFilter(enable_steep_filter);
        player_->SetResampler(samplerate, std::move(resampler));
    }
    else {
        player_->EnableResampler(false);
    }
}

void Xamp::processMeatadata(const std::vector<xamp::base::Metadata>& medata) {
    MetadataExtractAdapter::processMetadata(medata);
    emit album_artist_page_->album()->refreshOnece();
}

void Xamp::playMusic(const MusicEntity& item) {
    auto open_done = false;
        
    ui.seekSlider->setEnabled(true);

    try {
        player_->Open(item.file_path.toStdWString(), item.file_ext.toStdWString(), device_info_);
        setupResampler();
        player_->PlayStream();
        open_done = true;
    }
    catch (const xamp::base::Exception & e) {
        resetSeekPosValue();
        ui.seekSlider->setEnabled(false);        
        player_->Stop(false, true);
        XAMP_LOG_DEBUG("Exception: {}", e.GetErrorMessage());
        Toast::showTip(Q_UTF8(e.GetErrorMessage()), this);
    }
    catch (const std::exception & e) {
        resetSeekPosValue();
        ui.seekSlider->setEnabled(false);
        player_->Stop(false, true);
        Toast::showTip(Q_UTF8(e.what()), this);
    }
    catch (...) {
        resetSeekPosValue();
        ui.seekSlider->setEnabled(false);
        player_->Stop(false, true);
        Toast::showTip(tr("uknown error"), this);
    }

    if (!open_done) {
        return;
    }    

    if (!player_->IsMute()) {
        setVolume(ui.volumeSlider->value());
    }
    else {
        setVolume(0);
    }

    auto output_format = player_->GetOutputFormat();
    lrc_page_->spectrum()->setFrequency(100, output_format.GetSampleRate(), output_format.GetSampleRate());
    lrc_page_->spectrum()->start();

    ui.seekSlider->setRange(0, int32_t(player_->GetDuration() * 1000));
    ui.endPosLabel->setText(Time::msToString(player_->GetDuration()));
    playlist_page_->format()->setText(getPlayEntityFormat(player_.get(), item.file_ext));

    if (auto cover = PixmapCache::instance().find(item.cover_id)) {
        setCover(*cover.value());
        playlist_page_->cover()->setPixmap(Pixmap::resizeImage(*cover.value(),
            playlist_page_->cover()->size(), true));
    }
    else {
        setCover(ThemeManager::instance().pixmap().unknownCover());
    }

    ThemeManager::instance().setPlayOrPauseButton(ui, true);

    ui.titleLabel->setText(item.title);
    ui.artistLabel->setText(item.artist);

#if 1
    const QFileInfo file_info(item.file_path);
    const auto lrc_path = file_info.path()
        + Q_UTF8("/")
        + file_info.completeBaseName()
        + Q_UTF8(".lrc");
    lrc_page_->lyricsWidget()->loadLrcFile(lrc_path);
#endif

    playlist_page_->title()->setText(item.title);
    lrc_page_->title()->setText(item.title);
    lrc_page_->album()->setText(item.album);
    lrc_page_->artist()->setText(item.artist);

    Database::instance().addPlaybackHistory(item.album_id, item.artist_id, item.music_id);
    playback_history_page_->refreshOnece();
}

void Xamp::play(const PlayListEntity& item) {  
    current_entiry_ = item;
    playMusic(toMusicEntity(item));
}

void Xamp::play(const QModelIndex&, const PlayListEntity& item) {
    playLocalFile(item);
    if (!player_->IsPlaying()) {
        playlist_page_->format()->setText(Q_UTF8(""));
    }
}

void Xamp::onArtistIdChanged(const QString& artist, const QString& cover_id, int32_t artist_id) {
    artist_info_page_->setArtistId(artist, Database::instance().getArtistCoverId(artist_id), artist_id);
    ui.currentView->setCurrentWidget(artist_info_page_);
}

void Xamp::addPlaylistItem(const PlayListEntity &entity) {
    auto playlist_view = playlist_page_->playlist();    
    Database::instance().addMusicToPlaylist(entity.music_id, playlist_view->playlistId());
    playlist_view->appendItem(entity);
}

void Xamp::setCover(const QPixmap& cover) {
    assert(!cover.isNull());
    ui.coverLabel->setPixmap(Pixmap::resizeImage(cover, ui.coverLabel->size(), true));	
	
	if (cover.size().height() != cover.size().width()) {
		playlist_page_->cover()->setPixmap(Pixmap::resizeImage(cover, playlist_page_->cover()->size(), true));
	}
	else {
		playlist_page_->cover()->setBaseSize(ThemeManager::instance().getDefaultCoverSize());
		playlist_page_->cover()->setPixmap(Pixmap::resizeImage(cover, ThemeManager::instance().getDefaultCoverSize(), true));
	}
    lrc_page_->cover()->setPixmap(Pixmap::resizeImage(cover, lrc_page_->cover()->size(), true));
}

void Xamp::onPlayerStateChanged(xamp::player::PlayerState play_state) {
    if (!player_) {
        return;
    }
    if (play_state == PlayerState::PLAYER_STATE_STOPPED) {
        resetTaskbarProgress();
        ui.seekSlider->setValue(0);
        ui.startPosLabel->setText(Time::msToString(0));
        playNextItem(1);
        emit payNextMusic();
    }
}

void Xamp::addTable() {
    bool isOK = false;
    const auto table_name = QInputDialog::getText(nullptr, tr("Input Table Name"),
                                                  tr("Please input your name"),
                                                  QLineEdit::Normal,
                                                  tr("My favorite"),
                                                  &isOK);
    if (!isOK) {
        return;
    }
}

void Xamp::initialPlaylist() {
    Database::instance().forEachTable([&](auto table_id, auto /*table_index*/, auto playlist_id, const auto &name) {
        if (name.isEmpty()) {
            return;
        }

        ui.sliderBar->addTab(name, table_id);

        if (!playlist_page_) {
            playlist_page_ = newPlaylist(playlist_id);
            playlist_page_->playlist()->setPlaylistId(playlist_id);
            Database::instance().forEachPlaylistMusic(playlist_id, [this](const auto& entityy) {
                playlist_page_->playlist()->appendItem(entityy);
            });
			return;
        }

		if (playlist_page_->playlist()->playlistId() != playlist_id) {
            playlist_page_ = newPlaylist(playlist_id);
            playlist_page_->playlist()->setPlaylistId(playlist_id);
            Database::instance().forEachPlaylistMusic(playlist_id, [this](const auto& entityy) {
                playlist_page_->playlist()->appendItem(entityy);
            });
        }
    });

    if (!playlist_page_) {
        int32_t playlist_id = 1;
        if (!Database::instance().isPlaylistExist(playlist_id)) {
            playlist_id = Database::instance().addPlaylist(Q_UTF8(""), 0);
        }
        playlist_page_ = newPlaylist(playlist_id);
        playlist_page_->playlist()->setPlaylistId(playlist_id);
        Database::instance().forEachPlaylistMusic(playlist_id, [this](const auto& entityy) {
            playlist_page_->playlist()->appendItem(entityy);
        });
    }

    lrc_page_ = new LrcPage(this);
    album_artist_page_ = new AlbumArtistPage(this);
    playback_history_page_ = new PlaybackHistoryPage(this);
    playback_history_page_->setFont(font());
    playback_history_page_->hide();

    artist_info_page_ = new ArtistInfoPage(this);
    
    pushWidget(lrc_page_);    
    pushWidget(playlist_page_);
    pushWidget(album_artist_page_);
    pushWidget(artist_info_page_);
    goBackPage();
    goBackPage();

    (void)QObject::connect(state_adapter_.get(),
        &PlayerStateAdapter::onGetMagnitudeSpectrum,
        lrc_page_->spectrum(),
        &Spectrograph::onGetMagnitudeSpectrum);

    (void)QObject::connect(album_artist_page_->album(), &AlbumView::clickedArtist,
        this,
        &Xamp::onArtistIdChanged);

    (void)QObject::connect(this, &Xamp::textColorChanged,
        album_artist_page_->album(),
        &AlbumView::onTextColorChanged);

    (void)QObject::connect(this, &Xamp::textColorChanged,
        artist_info_page_,
        &ArtistInfoPage::onTextColorChanged);

    (void)QObject::connect(&mbc_, &MusicBrainzClient::finished, [this](auto artist_id, auto discogs_artist_id) {
        Database::instance().updateDiscogsArtistId(artist_id, discogs_artist_id);
        if (!discogs_artist_id.isEmpty()) {
            discogs_.searchArtistId(artist_id, discogs_artist_id);
        }
        });

    (void)QObject::connect(&discogs_, &DiscogsClient::getArtistImageUrl, [this](auto artist_id, auto url) {
        discogs_.downloadArtistImage(artist_id, url);
        XAMP_LOG_DEBUG("Download artist id: {}, discogs image url: {}", artist_id, url.toStdString());
        });

    (void)QObject::connect(&discogs_, &DiscogsClient::downloadImageFinished, [this](auto artist_id, auto image) {
        auto cover_id = PixmapCache::instance().add(image);
        Database::instance().updateArtistCoverId(artist_id, cover_id);
        XAMP_LOG_DEBUG("Save artist id: {} image, cover id : {}", artist_id, cover_id.toStdString());
        });

    (void)QObject::connect(album_artist_page_->album(), &AlbumView::addPlaylist, [this](const auto& entity) {
        addPlaylistItem(entity);
        });

    (void)QObject::connect(album_artist_page_->album(), &AlbumView::playMusic, [this](const auto& entity) {
        (void)QObject::disconnect(this, &Xamp::payNextMusic, playback_history_page_, &PlaybackHistoryPage::playNextMusic);
        (void)QObject::connect(this, &Xamp::payNextMusic, album_artist_page_->album(), &AlbumView::payNextMusic);
        playMusic(entity);
        });
    (void)QObject::connect(playback_history_page_, &PlaybackHistoryPage::playMusic, [this](const auto& entity) {
        (void)QObject::disconnect(this, &Xamp::payNextMusic, album_artist_page_->album(), &AlbumView::payNextMusic);
        (void)QObject::connect(this, &Xamp::payNextMusic, playback_history_page_, &PlaybackHistoryPage::playNextMusic);
        playMusic(entity);
        });

    setupPlayNextMusicSignals(true);

    auto space_key = new QShortcut(QKeySequence(Qt::Key_Space), this);
    (void)QObject::connect(space_key, &QShortcut::activated, [this]() {
        play();
        });
}

void Xamp::setupPlayNextMusicSignals(bool add_or_remove) {
    if (add_or_remove) {
        (void)QObject::connect(this, &Xamp::payNextMusic, album_artist_page_->album(), &AlbumView::payNextMusic);
        (void)QObject::connect(this, &Xamp::payNextMusic, playback_history_page_, &PlaybackHistoryPage::playNextMusic);
    }
    else {
        (void)QObject::disconnect(this, &Xamp::payNextMusic, album_artist_page_->album(), &AlbumView::payNextMusic);
        (void)QObject::disconnect(this, &Xamp::payNextMusic, playback_history_page_, &PlaybackHistoryPage::playNextMusic);
    }    
}

void Xamp::addItem(const QString& file_name) {
    auto add_playlist = dynamic_cast<PlyalistPage*>(ui.currentView->currentWidget()) != nullptr;

    if (add_playlist) {
        try {
            playlist_page_->playlist()->append(file_name);
            album_artist_page_->refreshOnece();
        }
        catch (const xamp::base::Exception & e) {
            Toast::showTip(Q_UTF8(e.GetErrorMessage()), this);
        }
    }
    else {
        auto adapter = new MetadataExtractAdapter();
        (void)QObject::connect(adapter, &MetadataExtractAdapter::readCompleted, this, &Xamp::processMeatadata);
        
        using namespace xamp::metadata;

        auto extract_handler = [adapter](const auto& file_name) {
            const Path path(file_name.toStdWString());
            TaglibMetadataReader reader;
            try {
                FromPath(path, adapter, &reader);
            }
            catch (const std::exception& e) {
                XAMP_LOG_DEBUG("FromPath has exception: {}", e.what());
            }
        };

        auto future = QtConcurrent::run(extract_handler, file_name);
        auto watcher = new QFutureWatcher<void>();
        (void)QObject::connect(watcher, &QFutureWatcher<void>::finished, [=]() {
            watcher->deleteLater();
            adapter->deleteLater();
            emit album_artist_page_->album()->refreshOnece();
            });

        watcher->setFuture(future);
    }
}

void Xamp::pushWidget(QWidget* widget) {
    auto id = ui.currentView->addWidget(widget);
    stack_page_id_.push(id);
    ui.currentView->setCurrentIndex(id);
}

QWidget* Xamp::topWidget() {
    if (!stack_page_id_.isEmpty()) {
        return ui.currentView->widget(stack_page_id_.top());
    }
    return nullptr;
}

QWidget* Xamp::popWidget() {
    if (!stack_page_id_.isEmpty()) {
        auto id = stack_page_id_.pop();
        auto widget = ui.currentView->widget(id);
        ui.currentView->removeWidget(widget);
        if (!stack_page_id_.isEmpty()) {
            ui.currentView->setCurrentIndex(stack_page_id_.top());
            return widget;
        }
    }
    return nullptr;
}

PlyalistPage* Xamp::newPlaylist(int32_t playlist_id) {
    auto playlist_page = new PlyalistPage(this);
    ui.currentView->addWidget(playlist_page);
    (void)QObject::connect(playlist_page->playlist(), &PlayListTableView::playMusic,
                           [this](auto index, const auto& item) {
        setupPlayNextMusicSignals(false);
        play(index, item);
    });
    (void)QObject::connect(playlist_page->playlist(), &PlayListTableView::removeItems,
                           [](auto playlist_id, const auto& select_music_ids) {
        IgnoreSqlError(Database::instance().removePlaylistMusic(playlist_id, select_music_ids))
    });
    (void)QObject::connect(playlist_page->playlist(), &PlayListTableView::readFingerprint,
        [this](auto index, const auto& item) {
            using namespace xamp::player;
            QProgressDialog dialog(tr("Read '") + item.title + tr("' fingerprint"), tr("Cancel"), 0, 100);
            dialog.setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
            dialog.setFont(font());
            dialog.setWindowTitle(tr("Read progress dialog"));
            dialog.setWindowModality(Qt::WindowModal);
            dialog.setMinimumSize(QSize(500, 100));
            dialog.setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
            dialog.show();

            FingerprintInfo fingerprint_info;
            QByteArray fingerprint;
            Fingerprint result;

            try {
                result = ReadFingerprint(item.file_path.toStdWString(),
                    item.file_ext.toStdWString(),
                                         [&](auto progress) -> bool {
                    dialog.setValue(progress);
                    qApp->processEvents();
                    return dialog.wasCanceled() != true;
                    });
            }
            catch (...) {
                return;
            }

            fingerprint.append(reinterpret_cast<char*>(result.fingerprint.data()), result.fingerprint.size());
            
            Database::instance().updateMusicFingerprint(item.music_id, fingerprint_info.fingerprint);

            fingerprint_info.artist_id = item.artist_id;
            fingerprint_info.duration = result.duration;            
            fingerprint_info.fingerprint = QString::fromLatin1(fingerprint);                        
            mbc_.searchBy(fingerprint_info);
    });

    (void)QObject::connect(this, &Xamp::textColorChanged,
        playlist_page->playlist(),
        &PlayListTableView::onTextColorChanged);
    playlist_page->playlist()->setPlaylistId(playlist_id);
    return playlist_page;
}

void Xamp::addDropFileItem(const QUrl& url) {
    addItem(url.toLocalFile());
}
