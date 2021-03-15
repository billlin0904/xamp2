//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QStack>
#include <QSystemTrayIcon>

#include <widget/widget_shared.h>

#include <widget/discogsclient.h>
#include <widget/appsettings.h>
#include <widget/uiplayerstateadapter.h>
#include <widget/framelesswindow.h>
#include <widget/musicbrainzclient.h>
#include <widget/playlistentity.h>
#include <widget/localelanguage.h>
#include <widget/playerorder.h>
#include <widget/musicentity.h>

#include "ui_xamp.h"

class LrcPage;
class PlyalistPage;
class AlbumView;
class ArtistView;
class AlbumArtistPage;
class ArtistInfoPage;
class PlaybackHistoryPage;
class QWidgetAction;
struct PlaybackFormat;

class Xamp final : public FramelessWindow {
	Q_OBJECT

public:
    explicit Xamp(QWidget *parent = nullptr);

signals:
	void payNextMusic();

    void themeChanged(QColor backgroundColor, QColor color);

public slots:
    void playMusic(const MusicEntity& item);

	void play(const QModelIndex& index, const PlayListEntity& item);

    void addPlaylistItem(const PlayListEntity &entity);

	void onArtistIdChanged(const QString& artist, const QString& cover_id, int32_t artist_id);

	void processMeatadata(const std::vector<Metadata>& medata) const;

	void onActivated(QSystemTrayIcon::ActivationReason reason);

    void onGaplessPlay(const QModelIndex &index);

	void onDisplayChanged(std::vector<float> const& display);
private:
    void initial();

	void applyTheme(QColor color);

	void setDefaultStyle();

	void setCover(const QPixmap* cover);

	void closeEvent(QCloseEvent* event) override;

	QWidgetAction* createTextSeparator(const QString& text);

	void onSampleTimeChanged(double stream_time);

	void playLocalFile(const PlayListEntity& item);

    void stopPlayedClicked() override;

    void playNextClicked() override;

    void playPreviousClicked() override;

    void play() override;

	void play(const PlayListEntity& item);

	void onPlayerStateChanged(PlayerState play_state);

	void addItem(const QString& file_name);

    void addTable();

	void addDropFileItem(const QUrl& url) override;

	void setVolume(int32_t volume);

	void initialUI();

	void initialPlaylist();

	void initialController();

	void initialDeviceList();

	void initialShortcut();

	void playNextItem(int32_t forward);

    void setTablePlaylistView(int table_id);

	void setPlayerOrder();

	void deleteKeyPress() override;

	PlyalistPage* newPlaylist(int32_t playlist_id);

	void pushWidget(QWidget* widget);

	QWidget* popWidget();

	QWidget* topWidget();

	void goBackPage();

	void getNextPage();

	void setSeekPosValue(double stream_time_as_ms);

	void resetSeekPosValue();

	void setupPlayNextMusicSignals(bool add_or_remove);

	void setupSampleRateConverter(bool enable_dsp);

	void registerMetaType();

    void onDeviceStateChanged(DeviceState state);

    void readFingerprint(const QModelIndex&, const PlayListEntity& item);

	void readFileLUFS(const QModelIndex&, const PlayListEntity& item);

	void exportWaveFile(const QModelIndex&, const PlayListEntity& item);

	void createTrayIcon();

    void updateUI(const MusicEntity& item, const PlaybackFormat& playback_format, bool open_done);

    void addPlayQueue();

	void applyConfig();

	bool is_seeking_;
	std::pair<double, double> loop_time{0,0};
	PlayerOrder order_;
	QModelIndex play_index_;
	DeviceInfo device_info_;
    PlayListEntity current_entity_;
	LrcPage* lrc_page_;
	PlyalistPage* playlist_page_;
	AlbumArtistPage* album_artist_page_;
	ArtistInfoPage* artist_info_page_;
	QStack<int32_t> stack_page_id_;	    
	std::shared_ptr<UIPlayerStateAdapter> state_adapter_;
	std::shared_ptr<AudioPlayer> player_;
	PlaybackHistoryPage* playback_history_page_;
	MusicBrainzClient mbc_;
	DiscogsClient discogs_;
	QMenu* tray_icon_menu_{};
	QSystemTrayIcon* tray_icon_{};
	VmMemLock player_lock_;
    Ui::XampWindow ui_{};
};
