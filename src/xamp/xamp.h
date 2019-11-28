//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QtWidgets/QMainWindow>
#include <QWidgetAction>
#include <QStack>
#include <QHash>
#include <set>

#include <output_device/devicefactory.h>
#include <player/audio_player.h>

#include "widget/appsettings.h"
#include "widget/playerstateadapter.h"
#include "widget/framelesswindow.h"
#include "widget/playlisttableview.h"
#include "widget/lyricsshowwideget.h"

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

class Xamp : public FramelessWindow {
	Q_OBJECT

public:
    Xamp(QWidget *parent = nullptr);

public slots:
	void play(const QModelIndex& index, const PlayListEntity& item);

private:
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

	bool is_seeking_;
	PlayerOrder order_;
	QModelIndex play_index_;
	Ui::XampWindow ui;
	DeviceInfo device_info_;
    PlayListEntity current_entiry_;
	LrcPage* lrc_page_;
	PlyalistPage* playlist_page_;
	QStack<int32_t> stack_page_id_;
    std::set<int32_t> music_id_store_;
	std::shared_ptr<PlayerStateAdapter> state_adapter_;
	std::shared_ptr<AudioPlayer> player_;
};
