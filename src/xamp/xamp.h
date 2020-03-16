//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <set>

#include <QtWidgets/QMainWindow>
#include <QWidgetAction>
#include <QStack>
#include <QHash>
#include <QTimer>

#include <output_device/devicefactory.h>
#include <player/audio_player.h>

#include <widget/discogsclient.h>
#include <widget/appsettings.h>
#include <widget/playerstateadapter.h>
#include <widget/framelesswindow.h>
#include <widget/playlisttableview.h>
#include <widget/lyricsshowwideget.h>
#include <widget/musicbrainzclient.h>
#include <widget/playbackhistorypage.h>
#include <widget/albumview.h>
#include <widget/filesystemwatcher.h>

#include "ui_xamp.h"

using namespace xamp::player;

enum PlayerOrder {
	PLAYER_ORDER_REPEAT_ONCE,
	PLAYER_ORDER_REPEAT_ONE,
	PLAYER_ORDER_SHUFFLE_ALL,
	_MAX_PLAYER_ORDER_,
};

class LrcPage;
class PlyalistPage;
class AlbumView;
class ArtistView;
class AlbumArtistPage;
class ArtistInfoPage;

class Xamp : public FramelessWindow {
	Q_OBJECT

public:
    explicit Xamp(QWidget *parent = nullptr);

signals:
	void payNextMusic();

	void textColorChanged(QColor backgroundColor, QColor color);

public slots:
    void playMusic(const MusicEntity& item);

	void play(const QModelIndex& index, const PlayListEntity& item);

    void addPlaylistItem(const PlayListEntity &entity);

	void onArtistIdChanged(const QString& artist, const QString& cover_id, int32_t artist_id);

	void processMeatadata(const std::vector<xamp::base::Metadata>& medata);
private:
	void applyTheme(QColor color);

	void setDefaultStyle();

	void setNightStyle();

	void setCover(const QPixmap& cover);

	void closeEvent(QCloseEvent* event) override;

	QWidgetAction* createTextSeparator(const QString& text);

	void onSampleTimeChanged(double stream_time);

	void playLocalFile(const PlayListEntity& item);

    void stopPlayedClicked() override;

    void playNextClicked() override;

    void playPreviousClicked() override;

    void play() override;

	void play(const PlayListEntity& item);

	void onPlayerStateChanged(xamp::player::PlayerState play_state);

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

	void setupResampler();

	void registerMetaType();

	void onDeviceStateChanged(DeviceState state);

	bool is_seeking_;
	PlayerOrder order_;
	QModelIndex play_index_;
	Ui::XampWindow ui;
	DeviceInfo device_info_;
    PlayListEntity current_entiry_;
	LrcPage* lrc_page_;
	PlyalistPage* playlist_page_;
	AlbumArtistPage* album_artist_page_;
	ArtistInfoPage* artist_info_page_;
	QStack<int32_t> stack_page_id_;	    
	std::shared_ptr<PlayerStateAdapter> state_adapter_;
	std::shared_ptr<AudioPlayer> player_;
	PlaybackHistoryPage* playback_history_page_;
	MusicBrainzClient mbc_;
	DiscogsClient discogs_;
	FileSystemWatcher watch_;
};
