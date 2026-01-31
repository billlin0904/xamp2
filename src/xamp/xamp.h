//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QSystemTrayIcon>
#include <QThread>
#include <optional>

#include <widget/widget_shared.h>
#include <widget/uiplayerstateadapter.h>
#include <xampplayer.h>
#include <ui_xamp.h>

class QWidgetAction;
class PlaybackQueueViewPage;
class FileSystemViewPage;
class LrcPage;

class Xamp final : public IXFrame {
	Q_OBJECT

public:
    Xamp(QWidget* parent, const std::shared_ptr<IAudioPlayer> &player);

    virtual ~Xamp() override;

    void setMainWindow(IXMainWindow* main_window);

    void addDropFileItem(const QUrl& url) override;

    void playPrevious() override;

    void playNext() override;

    void stopPlay() override;

    void playOrPause() override;

    void drivesChanges(const QList<DriveInfo>& drive_infos) override;

    void drivesRemoved(const DriveInfo& drive_info) override;

    void shortcutsPressed(const QKeySequence& shortcut) override;

	QString translateText(const std::string_view& text) override;

signals:
	
public slots:
    void onPlayerStateChanged(xamp::player::PlayerState play_state);

    void onSampleTimeChanged(double stream_time);
    
    void onDeviceStateChanged(DeviceState state, const QString& device_id);

private:
    void pushWidget(QWidget* widget);

    void setCurrentTab(int32_t table_id);

    void initialDeviceList(const std::string& device_id = "");

    QWidgetAction* createDeviceMenuWidget(const QString& desc);

    QString translateDeviceDescription(const IDeviceType* device_type);

    void showNaviBarButton();

	void setAlbumCover(const QPixmap& cover);

    void playLocalFile(const QString& file_name, bool queue = true);

    void setSeekPosValue(double stream_time);

    void playNextItem(int32_t forward);

	void setVolume(uint32_t volume);

    void onCheckForUpdate();

    void showAbout();

    void onActivated(QSystemTrayIcon::ActivationReason reason);

    bool is_seeking_{ false };
    IXMainWindow* main_window_{ nullptr };
    QAction* preference_action_{ nullptr };
    QScopedPointer<LrcPage> lrc_page_;
    QScopedPointer<FileSystemViewPage> file_explorer_page_;
    QScopedPointer<QSystemTrayIcon> tray_icon_;
	QScopedArrayPointer<PlaybackQueueViewPage> playback_queue_page_;
    QList<QFrame*> device_type_frame_;
    QList<QWidget*> widgets_;
    std::shared_ptr<IThreadPoolExecutor> thread_pool_;
    std::shared_ptr<UIPlayerStateAdapter> state_adapter_;
    std::shared_ptr<IAudioPlayer> player_;
    std::optional<DeviceInfo> device_info_;
	Ui::XampWindow ui_;
};
