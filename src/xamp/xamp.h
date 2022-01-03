//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QStack>
#include <QThread>
#include <QSystemTrayIcon>

#include <widget/widget_shared.h>
#include <widget/appsettings.h>
#include <widget/uiplayerstateadapter.h>
#include <widget/framelesswindow.h>
#include <widget/playlistentity.h>
#include <widget/localelanguage.h>
#include <widget/playerorder.h>
#include <widget/replaygainworker.h>
#include <widget/musicentity.h>
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
struct PlaybackFormat;
class YtMusicWebEngineView;

class Xamp final : public IXampPlayer {
	Q_OBJECT

public:
    Xamp();

    virtual ~Xamp() override = default;

    static void registerMetaType();

    void initial(ITopWindow *top_window);
signals:
	void payNextMusic();

    void themeChanged(QColor backgroundColor, QColor color);

	void nowPlaying(QString const& artist, QString const& title);

public slots:
    void playMusic(const MusicEntity& item);

	void play(const QModelIndex& index, const PlayListEntity& item);

    void addPlaylistItem(const std::vector<int32_t>& music_ids, const std::vector<PlayListEntity>& entities);

	void onArtistIdChanged(const QString& artist, const QString& cover_id, int32_t artist_id);

	void processMeatadata(const std::vector<Metadata>& medata) const;

	void onActivated(QSystemTrayIcon::ActivationReason reason);

	void onVolumeChanged(float volume);

private:
    void stopPlayedClicked() override;

    void playNextClicked() override;

    void playPreviousClicked() override;

    void play() override;

    void deleteKeyPress() override;

    void addDropFileItem(const QUrl& url) override;

	void applyTheme(QColor backgroundColor, QColor color);

	void changeTheme();

	void setCover(const QPixmap* cover, PlaylistPage *page = nullptr);

	void closeEvent(QCloseEvent* event) override;

	QWidgetAction* createTextSeparator(const QString& text);

	void onSampleTimeChanged(double stream_time);

	void playLocalFile(const PlayListEntity& item);

	void play(const PlayListEntity& item);

	void onPlayerStateChanged(PlayerState play_state);

	void addItem(const QString& file_name);

    void addTable();

	void setVolume(int32_t volume);

	void initialUI();

	void initialPlaylist();

	void initialController();

	void initialDeviceList();

	void initialShortcut();

	void playNextItem(int32_t forward);

    void setTablePlaylistView(int table_id);

	void setPlayerOrder();

	PlaylistPage* newPlaylist(int32_t playlist_id);

	void pushWidget(QWidget* widget);

	QWidget* popWidget();

	QWidget* topWidget();

	void goBackPage();

	void getNextPage();

	void setSeekPosValue(double stream_time_as_ms);

	void resetSeekPosValue();

	void setupPlayNextMusicSignals(bool add_or_remove);

    void onDeviceStateChanged(DeviceState state);

    void exportWaveFile(const PlayListEntity& item);

    void encodeFlacFile(const PlayListEntity& item);

	void createTrayIcon();

    void updateUI(const MusicEntity& item, const PlaybackFormat& playback_format, bool open_done);

	void updateButtonState();

	void extractFile(const QString &file_path);

	PlaylistPage* currentPlyalistPage();

	QString translasteError(Errors error);

	void cleanup();

	void avoidRedrawOnResize();

	bool is_seeking_;
	PlayerOrder order_;
	QModelIndex play_index_;
	DeviceInfo device_info_;
    PlayListEntity current_entity_;
	LrcPage* lrc_page_;
	PlaylistPage* playlist_page_;
	PlaylistPage* podcast_page_;
	PlaylistPage* current_playlist_page_;
	AlbumArtistPage* album_artist_page_;
    ArtistInfoPage* artist_info_page_;
	PreferencePage* preference_page_;
	AboutPage* about_page_;
    QMenu* tray_icon_menu_;
    QSystemTrayIcon* tray_icon_;
	QStack<int32_t> stack_page_id_;	    	
	YtMusicWebEngineView* ytmusic_view_;
    ReplayGainWorker replay_gain_worker_;
    QThread replay_gain_thread_;
	std::shared_ptr<UIPlayerStateAdapter> state_adapter_;
	std::shared_ptr<IAudioPlayer> player_;
    DicordNotify discord_notify_;
	QAction* search_action_;
	QAction* dark_mode_action_;
	QAction* light_mode_action_;
	QMenu* theme_menu_;
    ITopWindow *top_window_;
    Ui::XampWindow ui_;
};
