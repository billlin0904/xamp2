//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QThread>
#include <optional>

#include <widget/widget_shared.h>
#include <widget/uiplayerstateadapter.h>
#include <widget/playlistentity.h>
#include <xampplayer.h>
#include <ui_xamp.h>

class BackgroundService;
class FileSystemService;
class FileSystemViewPage;
class AlbumCoverService;
class RichPlaylistPage;
class LrcPage;
class CdPage;
class DeviceSelectorMenu;

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

    void destory() override;
signals:
    void fetchCdInfo(const DriveInfo& drive);

    void searchLyrics(const PlayListEntity& keyword);

public slots:
    void onPlayerStateChanged(xamp::player::PlayerState play_state);

    void onSampleTimeChanged(double stream_time);
    
    void onDeviceStateChanged(DeviceState state, const QString& device_id);

    void onUpdateCdTrackInfo(const QString& disc_id, const std::forward_list<TrackInfo>& track_infos);

    void OnReadMusicBrainzAlbums(const QList<PlayListEntity>& entities);
private:
    void pushWidget(QWidget* widget);

    void setCurrentTab(int32_t table_id);

    void initialDeviceList(const std::string& device_id = "");

    void showNaviBarButton();

	void setAlbumCover(const QPixmap& cover);

    void playLocalFile(const QString& file_name, bool queue = true, const PlayListEntity* entity = nullptr);

    void playLocalFile(const PlayListEntity& entity, bool queue = true);

    void setSeekPosValue(double stream_time);

    void playNextItem(int32_t forward);

	void setVolume(uint32_t volume);

    void configureUpdater(bool notify_on_finish);

    void installDownloadedUpdate(const QString& url, const QString& filepath);

    void setupSystemMenu();

    void showPreference();

    void showLogViewer();

    void onCheckForUpdate();

    void showAbout();

    bool is_seeking_{ false };
    IXMainWindow* main_window_{ nullptr };
    QAction* preference_action_{ nullptr };
    QScopedPointer<LrcPage> lrc_page_;
    QScopedPointer<RichPlaylistPage> rich_playlist_page_;
    QScopedPointer<FileSystemViewPage> file_explorer_page_;
	QScopedPointer<CdPage> cd_page_;
    QScopedPointer<DeviceSelectorMenu> device_menu_;
    QScopedPointer<FileSystemService> file_system_service_;
    QScopedPointer<AlbumCoverService> album_cover_service_;
    QScopedPointer<BackgroundService> background_service_;
    QList<QWidget*> widgets_;
    std::shared_ptr<IThreadPoolExecutor> thread_pool_;
    std::shared_ptr<UIPlayerStateAdapter> state_adapter_;
    std::shared_ptr<IAudioPlayer> player_;
    std::optional<DeviceInfo> device_info_;
    QThread file_system_service_thread_;
    QThread album_cover_service_thread_;
    QThread background_service_thread_;
	Ui::XampWindow ui_;
};
