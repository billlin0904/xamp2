#pragma once

#include <QtWidgets/QMainWindow>
#include <QWidgetAction>

#include <output_device/devicefactory.h>
#include <player/audio_player.h>

#include "widget/playerstateadapter.h"
#include "widget/framelesswindow.h"
#include "widget/playlisttableview.h"

#include "ui_xamp.h"

using namespace xamp::player;

class Xamp : public FramelessWindow {
	Q_OBJECT

public:
	Xamp(QWidget *parent = Q_NULLPTR);

public slots:
	void play(const QModelIndex& index, const PlayListEntity& item);

private:
	QWidgetAction* createTextSeparator(const QString& text);

	void setPlayOrPauseButton(bool is_playing);

	void onSampleTimeChanged(double stream_time);

	void playLocalFile(const PlayListEntity& item);

	void playNextClicked();

	void playPreviousClicked();

	void play(const PlayListEntity& item);	

	void onPlayerStateChanged(xamp::player::PlayerState play_state);

	void addItem(const QString& file_name);

	void addDropFileItem(const QUrl& url) override;

	void addMusic(int32_t music_id, PlayListTableView* playlist);

	void setVolume(int32_t volume);

	void initialUI();

	void initialController();

	void initialDeviceList();

	bool is_seeking_;
	Ui::XampWindow ui;
	DeviceInfo device_info_;
	std::shared_ptr<PlayerStateAdapter> state_adapter_;
	std::shared_ptr<AudioPlayer> player_;
};
