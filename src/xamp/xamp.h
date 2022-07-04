//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QStack>
#include <QThread>
#include <QSystemTrayIcon>
#include <QMainWindow>

#include <widget/widget_shared.h>
#include <widget/uiplayerstateadapter.h>
#include <widget/playlistentity.h>
#include <widget/playerorder.h>
#include <widget/backgroundworker.h>
#include <widget/albumentity.h>
#include <widget/discordnotify.h>

#include "xampplayer.h"
#include "ui_xamp.h"

class LrcPage;
class PlaylistPage;
class AboutPage;
class PreferencePage;
class AlbumView;
class ArtistView;
class AlbumArtistPage;
class ArtistInfoPage;
class PlaybackHistoryPage;
class QWidgetAction;
class QFileSystemWatcher;
class FileSystemViewPage;
struct PlaybackFormat;

class Xamp final : public IXampPlayer {
	Q_OBJECT

public:
    Xamp();

    virtual ~Xamp() override;

    void setXWindow(IXWindow* top_window);

    void applyTheme(QColor backgroundColor, QColor color);
signals:
	void payNextMusic();

    void themeChanged(QColor backgroundColor, QColor color);

	void nowPlaying(QString const& artist, QString const& title);

	void addBlurImage(const QString& cover_id, const QImage& image);

public slots:
    void playAlbumEntity(const AlbumEntity& item);

	void playPlayListEntity(const PlayListEntity& item);

    void addPlaylistItem(const Vector<int32_t>& music_ids, const Vector<PlayListEntity>& entities);

	void onArtistIdChanged(const QString& artist, const QString& cover_id, int32_t artist_id);

	void processMeatadata(int64_t dir_last_write_time, const ForwardList<Metadata>& medata) const;

	void onActivated(QSystemTrayIcon::ActivationReason reason);

	void onVolumeChanged(float volume);

	void setCover(const QString& cover_id, PlaylistPage* page);

private:
    void stopPlayedClicked() override;

    void playNextClicked() override;

    void playPreviousClicked() override;

    void play() override;

    void deleteKeyPress() override;

    void addDropFileItem(const QUrl& url) override;

	void closeEvent(QCloseEvent* event) override;

	void setPlaylistPageCover(const QPixmap* cover, PlaylistPage* page = nullptr);

	QWidgetAction* createTextSeparator(const QString& text);

	void onSampleTimeChanged(double stream_time);

	void playLocalFile(const PlayListEntity& item);

	void onPlayerStateChanged(PlayerState play_state);

	void addItem(const QString& file_name);

	void setVolume(int32_t volume);

	void initialUI();

	void initialPlaylist();

	void initialController();

	void initialDeviceList();

	void initialShortcut();

	void playNextItem(int32_t forward);

    void setTablePlaylistView(int table_id);

	void setPlayerOrder();

	PlaylistPage* newPlaylistPage(int32_t playlist_id);

	void pushWidget(QWidget* widget);

	QWidget* popWidget();

	QWidget* topWidget();

	void goBackPage();

	void getNextPage();

	void setSeekPosValue(double stream_time_as_ms);

	void resetSeekPosValue();

    void onDeviceStateChanged(DeviceState state);

    void encodeFlacFile(const PlayListEntity& item);

	void createTrayIcon();

    void updateUI(const AlbumEntity& item, const PlaybackFormat& playback_format, bool open_done);

	void updateButtonState();

	void extractFile(const QString &file_path);

	PlaylistPage* currentPlyalistPage();

	void cleanup();

    void setupDSP(const AlbumEntity& item);

	void avoidRedrawOnResize();

	void connectSignal(PlaylistPage* playlist_page);

	bool is_seeking_;
	PlayerOrder order_;
	QModelIndex play_index_;
	DeviceInfo device_info_;
    PlayListEntity current_entity_;
	LrcPage* lrc_page_;
	PlaylistPage* playlist_page_;
	PlaylistPage* podcast_page_;
	PlaylistPage* music_page_;
	PlaylistPage* current_playlist_page_;
	AlbumArtistPage* album_artist_page_;
    ArtistInfoPage* artist_info_page_;
	PreferencePage* preference_page_;
	FileSystemViewPage* file_system_view_page_;
	AboutPage* about_page_;
    QMenu* tray_icon_menu_;
    QSystemTrayIcon* tray_icon_;
    QStack<int32_t> stack_page_id_;
    BackgroundWorker background_worker_;
    QThread background_thread_;
	std::shared_ptr<UIPlayerStateAdapter> state_adapter_;
	std::shared_ptr<IAudioPlayer> player_;
    DicordNotify discord_notify_;
	QAction* search_action_;
	QAction* dark_mode_action_;
	QAction* light_mode_action_;
	QMenu* theme_menu_;
    IXWindow *top_window_;
    Ui::XampWindow ui_;
};
