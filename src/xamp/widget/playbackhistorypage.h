//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QTableView>
#include <QSqlQueryModel>

class PlaybackHistoryTableView : public QTableView {
	Q_OBJECT
public:
	enum {
		PLAYLIST_TRACK,
		PLAYLIST_TITLE,
		PLAYLIST_DURATION,
	};

	explicit PlaybackHistoryTableView(QWidget* parent = nullptr);

	void refreshOnece();
private:
	void resizeColumn();

	QSqlQueryModel model_;
};

class PlaybackHistoryPage : public QFrame {
	Q_OBJECT
public:
	explicit PlaybackHistoryPage(QWidget* parent = nullptr);

	PlaybackHistoryTableView* playlist() {
		return playlist_;
	}

	void refreshOnece();

signals:
	void save();

private:
	PlaybackHistoryTableView* playlist_;
};

