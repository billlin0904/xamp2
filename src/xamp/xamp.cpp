#include <QDebug>
#include <QToolTip>
#include <QMenu>
#include <QCloseEvent>
#include <QFileInfo>
#include <QDesktopWidget>
#include <QInputDialog>
#include <QShortcut>
#include <QScreen>
#include <QProgressDialog>
#include <QMainWindow>
#include <QWidgetAction>
#include <QSystemTrayIcon>

#include <base/scopeguard.h>
#include <base/str_utilts.h>

#include <output_device/devicefactory.h>

#include <player/chromaprinthelper.h>
#include <player/soxresampler.h>

#include <widget/albumview.h>
#include <widget/lyricsshowwideget.h>
#include <widget/playlisttableview.h>
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
#include <widget/jsonsettings.h>
#include <widget/eqdialog.h>
#include <widget/playbackhistorypage.h>
#include <widget/eqdialog.h>
#include <widget/ui_utilts.h>

#include "aboutdialog.h"
#include "preferencedialog.h"
#include "thememanager.h"
#include "xamp.h"

Xamp::Xamp(QWidget *parent)
    : FramelessWindow(parent)
    , is_seeking_(false)
    , order_(PlayerOrder::PLAYER_ORDER_REPEAT_ONCE)
    , lrc_page_(nullptr)
    , playlist_page_(nullptr)
    , album_artist_page_(nullptr)
    , artist_info_page_(nullptr)
    , state_adapter_(std::make_shared<PlayerStateAdapter>())
    , player_(std::make_shared<AudioPlayer>(state_adapter_))
    , playback_history_page_(nullptr) {    
    initial();
}

void Xamp::initial() {
    initialUI();
    initialController();
    initialDeviceList();
    initialPlaylist();
    initialShortcut();
    setCover(nullptr);
    DeviceManager::Instance().RegisterDeviceListener(player_);
    createTrayIcon();
    setDefaultStyle();    
}

void Xamp::onActivated(QSystemTrayIcon::ActivationReason reason) {
    switch (reason) {
    case QSystemTrayIcon::Trigger:
    {
        showNormal();
        raise();
        activateWindow();
        break;
    }
    case QSystemTrayIcon::DoubleClick:
    {
        break;
    }
    default:
        break;
    }
}

void Xamp::createTrayIcon() {
    auto minimizeAction = new QAction(tr("Mi&nimize"), this);
    QObject::connect(minimizeAction, &QAction::triggered, this, &QWidget::hide);

    auto  maximizeAction = new QAction(tr("Ma&ximize"), this);
    QObject::connect(maximizeAction, &QAction::triggered, this, &QWidget::showMaximized);

    auto restoreAction = new QAction(tr("&Restore"), this);
    QObject::connect(restoreAction, &QAction::triggered, this, &QWidget::showNormal);

    auto quitAction = new QAction(tr("&Quit"), this);
    QObject::connect(quitAction, &QAction::triggered, this, &QWidget::close);

    auto aboutAction = new QAction(tr("&About"), this);
    (void)QObject::connect(aboutAction, &QAction::triggered, [=]() {
        AboutDialog aboutdlg;
        aboutdlg.setFont(font());
        aboutdlg.exec();
        });

    trayIconMenu_ = new QMenu(this);
    trayIconMenu_->addAction(minimizeAction);
    trayIconMenu_->addAction(maximizeAction);
    trayIconMenu_->addAction(restoreAction);
    trayIconMenu_->addAction(aboutAction);
    trayIconMenu_->addSeparator();
    trayIconMenu_->addAction(quitAction);

    trayIcon_ = new QSystemTrayIcon(ThemeManager::instance().appIcon(), this);
    trayIcon_->setContextMenu(trayIconMenu_);
    trayIcon_->setToolTip(Q_UTF8("XAMP"));
    trayIcon_->show();

    QObject::connect(trayIcon_,
        SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
        this,
        SLOT(onActivated(QSystemTrayIcon::ActivationReason)));
}

//void Xamp::paintEvent(QPaintEvent* event) {
//    QPainter painter(this);
//    
//    auto cover = lrc_page_->cover()->pixmap();
//    if (cover != nullptr) {
//    	painter.drawPixmap(rect(), Pixmap::blurImage(*cover, 12));
//    }
//}

void Xamp::closeEvent(QCloseEvent* event) {
    if (trayIcon_->isVisible() && !isHidden()) {
        hide();
        event->ignore();
        return;
    }

    try {
        AppSettings::setValue(kAppSettingVolume, player_->GetVolume());
    } catch (...) {
    }

    AppSettings::setValue(kAppSettingWidth, size().width());
    AppSettings::setValue(kAppSettingHeight, size().height());
    AppSettings::setValue(kAppSettingVolume, ui.volumeSlider->value());

    if (player_ != nullptr) {
        player_->Destroy();
        player_.reset();
    }
}

void Xamp::setDefaultStyle() {
    ThemeManager::instance().setDefaultStyle(ui);
    applyTheme(ThemeManager::instance().getBackgroundColor());
}

void Xamp::registerMetaType() {
    qRegisterMetaType<std::vector<Metadata>>("std::vector<xamp::base::Metadata>");
    qRegisterMetaType<DeviceState>("xamp::output_device::DeviceState");
    qRegisterMetaType<PlayerState>("xamp::player::PlayerState");
    qRegisterMetaType<Errors>("xamp::base::Errors");
    qRegisterMetaType<std::vector<float>>("std::vector<float>");
    qRegisterMetaType<size_t>("size_t");
}

void Xamp::initialUI() {
    registerMetaType();    
    ui.setupUi(this);
    watch_.addPath(AppSettings::getValueAsString(kAppSettingMusicFilePath));
    auto f = font();
    f.setPointSize(10);
    ui.titleLabel->setFont(f);
    f.setPointSize(8);
    ui.artistLabel->setFont(f);
#ifdef Q_OS_WIN
    f.setPointSize(7);
    ui.startPosLabel->setFont(f);
    ui.endPosLabel->setFont(f);
#else
    ui.closeButton->hide();
    ui.maxWinButton->hide();
    ui.minWinButton->hide();
    f.setPointSize(11);
    ui.titleLabel->setFont(f);
    f.setPointSize(10);
    ui.artistLabel->setFont(f);
#endif
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

    menu->clear();

    DeviceInfo init_device_info;
    auto is_find_setting_device = false;

    auto device_action_group = new QActionGroup(this);
    device_action_group->setExclusive(true);

    std::map<std::string, QAction*> device_id_action;

    DeviceManager::Instance().ForEach([&](const auto &device_type) {
        device_type->ScanNewDevice();

        const auto device_info_list = device_type->GetDeviceInfo();
        if (device_info_list.empty()) {
            return;
        }

        menu->addAction(createTextSeparator(fromStdStringView(device_type->GetDescription())));

        for (const auto& device_info : device_info_list) {
            auto device_action = new QAction(QString::fromStdWString(device_info.name), this);
            device_action_group->addAction(device_action);
            device_action->setCheckable(true);
            device_id_action[device_info.device_id] = device_action;
            (void)QObject::connect(device_action, &QAction::triggered, [device_info, this]() {
                device_info_ = device_info;
                AppSettings::setValue(kAppSettingDeviceType, device_info_.device_type_id);
                AppSettings::setValue(kAppSettingDeviceId, device_info_.device_id);
                XAMP_LOG_DEBUG("Save device Id : {}", device_info_.device_id);
            });
            menu->addAction(device_action);
            if (AppSettings::getID(kAppSettingDeviceType) == device_info.device_type_id
                && AppSettings::getValueAsString(kAppSettingDeviceId).toStdString() == device_info.device_id) {
                device_info_ = device_info;
                is_find_setting_device = true;
                device_action->setChecked(true);
            }
        }

        if (!is_find_setting_device) {
            auto itr = std::find_if(device_info_list.begin(), device_info_list.end(), [](const auto& info) {
                return info.is_default_device && !DeviceManager::IsExclusiveDevice(info);
            });
            if (itr != device_info_list.end()) {
                init_device_info = (*itr);
            }
        }
    });

    if (!is_find_setting_device) {
        device_info_ = init_device_info;
        device_id_action[device_info_.device_id]->setChecked(true);
        AppSettings::setValue(kAppSettingDeviceType, device_info_.device_type_id);
        AppSettings::setValue(kAppSettingDeviceId, device_info_.device_id);
        XAMP_LOG_DEBUG("Use default device Id : {}", device_info_.device_id);
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
        if (!ui.volumeSlider->isEnabled()) {
            return;
        }
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

    order_ = static_cast<PlayerOrder>(AppSettings::getAsInt(kAppSettingOrder));
    setPlayerOrder();

    auto vol = AppSettings::getValue(kAppSettingVolume).toUInt();
    ui.volumeSlider->setRange(0, 100);
    ui.volumeSlider->setValue(static_cast<int32_t>(vol));
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
        order_ = GetNextOrder(order_);
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
        playback_history_page_->setMinimumSize(QSize(550, 400));
        playback_history_page_->refreshOnece();
        playback_history_page_->show();
    });

    (void)QObject::connect(ui.eqButton, &QToolButton::pressed, [this]() {
        EQDialog eqdialog;
        eqdialog.setFont(font());
        if (eqdialog.exec() == QDialog::Accepted) {
            eqsettings_ = eqdialog.EQSettings;
        }        
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
    auto settings_action = new QAction(tr("Settings"), this);
    settings_menu->addAction(settings_action);
    (void)QObject::connect(settings_action, &QAction::triggered, [=]() {
        PreferenceDialog dialog;
        dialog.setFont(font());
        dialog.exec();
        watch_.addPath(dialog.music_file_path_);
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
#ifdef Q_OS_WIN
    auto enable_blur_material_mode_action = new QAction(tr("Enable blur"), this);
    enable_blur_material_mode_action->setCheckable(true);
    if (AppSettings::getValue(kAppSettingEnableBlur).toBool()) {
        enable_blur_material_mode_action->setChecked(true);
    }

    (void)QObject::connect(enable_blur_material_mode_action, &QAction::triggered, [=]() {
        auto enable = AppSettings::getValueAsBool(kAppSettingEnableBlur);
        enable = !enable;
        enable_blur_material_mode_action->setChecked(enable);
        ThemeManager::instance().enableBlur(this, enable);
        });
    settings_menu->addAction(enable_blur_material_mode_action);
#endif
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
        emit themeChanged(color, Qt::black);
        ThemeManager::instance().setThemeColor(ThemeColor::WHITE_THEME);
    }
    else {
        emit themeChanged(color, Qt::white);
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
        player_->SetVolume(static_cast<uint32_t>(volume));
    }
    catch (const xamp::base::Exception& e) {
        player_->Stop(false);
        Toast::showTip(Q_UTF8(e.GetErrorMessage()), this);
    }
}

void Xamp::initialShortcut() {
    auto space_key = new QShortcut(QKeySequence(Qt::Key_Space), this);
    (void)QObject::connect(space_key, &QShortcut::activated, [this]() {
        play();
    });
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
        AppSettings::setValue(kAppSettingOrder,
                              static_cast<int>(PlayerOrder::PLAYER_ORDER_REPEAT_ONCE));
        break;
    case PlayerOrder::PLAYER_ORDER_REPEAT_ONE:
        ThemeManager::instance().setRepeatOnePlayorder(ui);
        AppSettings::setValue(kAppSettingOrder,
                              static_cast<int>(PlayerOrder::PLAYER_ORDER_REPEAT_ONE));
        break;
    case PlayerOrder::PLAYER_ORDER_SHUFFLE_ALL:
        ThemeManager::instance().setShufflePlayorder(ui);
        AppSettings::setValue(kAppSettingOrder,
                              static_cast<int>(PlayerOrder::PLAYER_ORDER_SHUFFLE_ALL));
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

    play_index_ = playlist_view->currentIndex();   

    if (count > 1) {
        switch (order_) {
        case PlayerOrder::PLAYER_ORDER_REPEAT_ONCE:
            play_index_ = playlist_view->nextIndex(forward);
            if (play_index_.row() == -1) {
                return;
            }
            break;
        case PlayerOrder::PLAYER_ORDER_SHUFFLE_ALL:
            play_index_ = playlist_view->shuffeIndex();
            break;
        case PlayerOrder::PLAYER_ORDER_REPEAT_ONE:
        default:
            break;
        }

        if (!play_index_.isValid()) {
            Toast::showTip(tr("Not found any playlist item."), this);
            return;
        }
    } else {
        play_index_ = playlist_view->model()->index(0, 0);
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
    if (AppSettings::getValue(kAppSettingResamplerEnable).toBool()) {
        auto soxr_settings = JsonSettings::getValue(AppSettings::getValueAsString(kAppSettingSoxrSettingName)).toMap();

        auto samplerate = soxr_settings[kSoxrResampleSampleRate].toUInt();
        auto quality = static_cast<SoxrQuality>(soxr_settings[kSoxrQuality].toInt());
        auto phase = static_cast<SoxrPhaseResponse>(soxr_settings[kSoxrPhase].toInt());
        auto passband = soxr_settings[kSoxrPassBand].toInt();
        auto enable_steep_filter = soxr_settings[kSoxrEnableSteepFilter].toBool();

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
    emit album_artist_page_->artist()->refreshOnece();    
}

void Xamp::setupEQ() {
    auto enable_eq = AppSettings::getValueAsBool(kEnableEQ);

    player_->EnableEQ(enable_eq);

    if (enable_eq) {
        auto eqName = AppSettings::getValueAsString(kEQName);
        if (eqName.isEmpty() || eqsettings_.isEmpty()) {
            eqsettings_ = AppSettings::getEQPreset().first();
        }

        uint32_t i = 0;
        for (auto band : eqsettings_) {
            player_->SetEQ(i++, band.gain, band.Q);
        }
    }    
}

void Xamp::playMusic(const MusicEntity& item) {
    auto open_done = false;

    ui.seekSlider->setEnabled(true);

    XAMP_ON_SCOPE_EXIT(
        if (open_done) {
            return;
        }
        resetSeekPosValue();
        ui.seekSlider->setEnabled(false);
        player_->Stop(false, true);
        );

    try {
        player_->Open(item.file_path.toStdWString(), item.file_ext.toStdWString(), device_info_);
        setupResampler();
        setupEQ();
        player_->StartPlay();
        open_done = true;
    }
    catch (const xamp::base::Exception & e) {
        XAMP_LOG_DEBUG("Exception: {}", e.GetErrorMessage());
        Toast::showTip(Q_UTF8(e.GetErrorMessage()), this);
    }
    catch (const std::exception & e) {
        Toast::showTip(Q_UTF8(e.what()), this);
    }
    catch (...) {
        Toast::showTip(tr("uknown error"), this);
    }

    if (!open_done) {
        return;
    }

    if (player_->CanHardwareControlVolume()) {
        if (!player_->IsMute()) {
            setVolume(ui.volumeSlider->value());
        }
        else {
            setVolume(0);
        }
        ui.volumeSlider->setDisabled(false);
    }
    else {
        ui.volumeSlider->setDisabled(true);
    }

    Database::instance().addPlaybackHistory(item.album_id, item.artist_id, item.music_id);
    playback_history_page_->refreshOnece();

    ui.seekSlider->setRange(0, int32_t(player_->GetDuration() * 1000));
    ui.endPosLabel->setText(Time::msToString(player_->GetDuration()));
    playlist_page_->format()->setText(format2String(player_.get(), item.file_ext));

    if (auto cover = PixmapCache::instance().find(item.cover_id)) {
        setCover(cover.value());
    }
    else {
        setCover(nullptr);
    }

    ThemeManager::instance().setPlayOrPauseButton(ui, true);

    ui.titleLabel->setText(item.title);
    ui.artistLabel->setText(item.artist);

    playlist_page_->title()->setText(item.title);

    const QFileInfo file_info(item.file_path);
    const auto lrc_path = file_info.path()
                          + Q_UTF8("/")
                          + file_info.completeBaseName()
                          + Q_UTF8(".lrc");
    lrc_page_->lyricsWidget()->loadLrcFile(lrc_path);
    lrc_page_->title()->setText(item.title);
    lrc_page_->album()->setText(item.album);
    lrc_page_->artist()->setText(item.artist);

    update();
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

void Xamp::onArtistIdChanged(const QString& artist, const QString& /*cover_id*/, int32_t artist_id) {
    artist_info_page_->setArtistId(artist, Database::instance().getArtistCoverId(artist_id), artist_id);
    ui.currentView->setCurrentWidget(artist_info_page_);
}

void Xamp::addPlaylistItem(const PlayListEntity &entity) {
    auto playlist_view = playlist_page_->playlist();    
    Database::instance().addMusicToPlaylist(entity.music_id, playlist_view->playlistId());
    playlist_view->appendItem(entity);
}

void Xamp::setCover(const QPixmap* cover) {
    if (!cover) {
        cover = &ThemeManager::instance().pixmap().unknownCover();
        lrc_page_->setBackground(QPixmap());        
    }
    else {
        lrc_page_->setBackground(*cover);
    }

    ui.coverLabel->setPixmap(Pixmap::resizeImage(*cover, ui.coverLabel->size(), true));
    playlist_page_->cover()->setPixmap(Pixmap::resizeImage(*cover, playlist_page_->cover()->size(), true));
    lrc_page_->cover()->setPixmap(Pixmap::resizeImage(*cover, lrc_page_->cover()->size(), true));
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
    Database::instance().forEachTable([this](auto table_id,
                                             auto /*table_index*/,
                                             auto playlist_id,
                                             const auto &name) {
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
    playback_history_page_->setObjectName(Q_UTF8("playbackHistoryPage"));
    playback_history_page_->setFont(font());
    playback_history_page_->hide();

    artist_info_page_ = new ArtistInfoPage(this);
    
    pushWidget(lrc_page_);    
    pushWidget(playlist_page_);
    pushWidget(album_artist_page_);
    pushWidget(artist_info_page_);
    goBackPage();
    goBackPage();

    (void)QObject::connect(album_artist_page_->album(),
                            &AlbumView::clickedArtist,
                            this,
                            &Xamp::onArtistIdChanged);

    (void)QObject::connect(this,
                            &Xamp::themeChanged,
                            album_artist_page_->album(),
                            &AlbumView::OnThemeColorChanged);

    (void)QObject::connect(this,
                            &Xamp::themeChanged,
                            artist_info_page_,
                            &ArtistInfoPage::OnThemeColorChanged);

    (void)QObject::connect(this,
                            &Xamp::themeChanged,
                            lrc_page_,
                            &LrcPage::OnThemeColorChanged);

    (void)QObject::connect(&mbc_, &MusicBrainzClient::finished,
                            [this](auto artist_id, auto discogs_artist_id) {
                                Database::instance().updateDiscogsArtistId(artist_id, discogs_artist_id);
                                if (!discogs_artist_id.isEmpty()) {
                                    discogs_.searchArtistId(artist_id, discogs_artist_id);
                                }
                            });

    (void)QObject::connect(&discogs_,
                            &DiscogsClient::getArtistImageUrl,
                            [this](auto artist_id, auto url) {
                                discogs_.downloadArtistImage(artist_id, url);
                                XAMP_LOG_DEBUG("Download artist id: {}, discogs image url: {}", artist_id, url.toStdString());
                            });

    (void)QObject::connect(&discogs_,
                            &DiscogsClient::downloadImageFinished,
                            [](auto artist_id, auto image) {
                                auto cover_id = PixmapCache::instance().add(image);
                                Database::instance().updateArtistCoverId(artist_id, cover_id);
                                XAMP_LOG_DEBUG("Save artist id: {} image, cover id : {}", artist_id, cover_id.toStdString());
                            });

    (void)QObject::connect(album_artist_page_->album(),
                            &AlbumView::addPlaylist,
                            [this](const auto& entity) {
                                addPlaylistItem(entity);
                            });

    (void)QObject::connect(album_artist_page_->album(),
                            &AlbumView::playMusic,
                            [this](const auto& entity) {
                                (void)QObject::disconnect(this, &Xamp::payNextMusic, playback_history_page_, &PlaybackHistoryPage::playNextMusic);
                                (void)QObject::connect(this, &Xamp::payNextMusic, album_artist_page_->album(), &AlbumView::payNextMusic);
                                playMusic(entity);
                            });
    (void)QObject::connect(playback_history_page_,
                            &PlaybackHistoryPage::playMusic,
                            [this](const auto& entity) {
                                (void)QObject::disconnect(this, &Xamp::payNextMusic, album_artist_page_->album(), &AlbumView::payNextMusic);
                                (void)QObject::connect(this, &Xamp::payNextMusic, playback_history_page_, &PlaybackHistoryPage::playNextMusic);
                                playMusic(entity);
                            });

    setupPlayNextMusicSignals(true);
}

void Xamp::setupPlayNextMusicSignals(bool add_or_remove) {
    if (add_or_remove) {
        (void)QObject::connect(this,
                                &Xamp::payNextMusic,
                                album_artist_page_->album(),
                                &AlbumView::payNextMusic);
        (void)QObject::connect(this,
                                &Xamp::payNextMusic,
                                playback_history_page_,
                                &PlaybackHistoryPage::playNextMusic);
    }
    else {
        (void)QObject::disconnect(this,
                                   &Xamp::payNextMusic,
                                   album_artist_page_->album(),
                                   &AlbumView::payNextMusic);
        (void)QObject::disconnect(this,
                                   &Xamp::payNextMusic,
                                   playback_history_page_,
                                   &PlaybackHistoryPage::playNextMusic);
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

        (void)QObject::connect(adapter,
            &MetadataExtractAdapter::readCompleted,
            this,
            &Xamp::processMeatadata,
            Qt::QueuedConnection);
        
        MetadataExtractAdapter::readMetadata(adapter, file_name);
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

void Xamp::readFingerprint(const QModelIndex&, const PlayListEntity& item) {
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

    try {
        const auto [duration, fingerprint_result] =
            ReadFingerprint(item.file_path.toStdWString(),
                            item.file_ext.toStdWString(),
                            [&](auto progress) -> bool {
                                dialog.setValue(progress);
                                qApp->processEvents();
                                return dialog.wasCanceled() != true;
                            });

        fingerprint.append(reinterpret_cast<char const *>(fingerprint_result.data()),
                           static_cast<int32_t>(fingerprint_result.size()));
        Database::instance().updateMusicFingerprint(item.music_id, fingerprint_info.fingerprint);
        fingerprint_info.duration = static_cast<int32_t>(duration);
    } catch (...) {
        return;
    }

    fingerprint_info.artist_id = item.artist_id;
    fingerprint_info.fingerprint = QString::fromLatin1(fingerprint);
    mbc_.searchBy(fingerprint_info);
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
                            this, &Xamp::readFingerprint);

    (void)QObject::connect(this, &Xamp::themeChanged,
                            playlist_page->playlist(),
                            &PlayListTableView::onTextColorChanged);

    playlist_page->playlist()->setPlaylistId(playlist_id);

    return playlist_page;
}

void Xamp::addDropFileItem(const QUrl& url) {
    addItem(url.toLocalFile());
}
