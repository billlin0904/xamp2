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

class PlaybackController;
class PlaybackPresenter;
class ApplicationUpdater;
class ApplicationServices;
class BackgroundService;
class FileSystemService;
class FileSystemViewPage;
class AlbumCoverService;
class RichPlaylistPage;
class LrcPage;
class CdPage;
class DeviceSelectorMenu;
class PreferencePage;

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

    void setupSystemMenu();
signals:
    void fetchCdInfo(const DriveInfo& drive);

    void searchLyrics(const PlayListEntity& keyword);

public slots:
    void onDeviceStateChanged(DeviceState state, const QString& device_id);

    void onUpdateCdTrackInfo(const QString& disc_id, const std::forward_list<TrackInfo>& track_infos);

    void OnReadMusicBrainzAlbums(const QList<PlayListEntity>& entities);

private:

    void setCurrentTab(int32_t table_id);

    void initialDeviceList(const std::string& device_id = "");

    void initializeModernControls();
    void refreshPlaylistNavigation(int selected_id);

	void setAlbumCover(const QPixmap& cover);

    void invalidateAlbumCover(int32_t music_id, int32_t album_id);

	void setVolume(uint32_t volume);

    void showPreference();

    void showLogViewer();

    void showEncodeJobs(int32_t encode_type, const QList<PlayListEntity>& entities);

    void onCheckForUpdate();

    void showAbout();

    QScopedPointer<PlaybackController> playback_;
    QScopedPointer<PlaybackPresenter> presenter_;
    QScopedPointer<ApplicationUpdater> updater_;
    std::unique_ptr<ApplicationServices> services_;
    IXMainWindow* main_window_{ nullptr };
    QAction* preference_action_{ nullptr };
    PreferencePage* preference_page_{ nullptr };
    QWidget* settings_panel_{ nullptr };
    QScopedPointer<LrcPage> lrc_page_;
    QScopedPointer<RichPlaylistPage> rich_playlist_page_;
    QScopedPointer<FileSystemViewPage> file_explorer_page_;
	QScopedPointer<CdPage> cd_page_;
    QScopedPointer<DeviceSelectorMenu> device_menu_;
    std::shared_ptr<IAudioPlayer> player_;
    std::optional<DeviceInfo> device_info_;
	Ui::XampWindow ui_;
};
