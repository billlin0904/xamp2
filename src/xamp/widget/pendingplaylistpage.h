//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QTableView>

class QSqlQueryModel;

class PendingPlayTableView : public QTableView {
public:
	explicit PendingPlayTableView(QWidget* parent = nullptr);

	void Reload();

	void SetPlaylistId(int32_t playlist_id);
private:
	QSqlQueryModel* model_;
};

class PendingPlaylistPage : public QFrame {
public:
	explicit PendingPlaylistPage(QWidget* parent = nullptr);

private:
	PendingPlayTableView* playlist_;
};
