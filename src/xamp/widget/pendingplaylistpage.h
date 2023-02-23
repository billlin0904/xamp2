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
	Q_OBJECT
public:
	explicit PendingPlaylistPage(const QList<QModelIndex>& indexes, QWidget* parent = nullptr);

signals:
	void PlayMusic(const QModelIndex& index);

private:
	PendingPlayTableView* playlist_;
	QList<QModelIndex> indexes_;
};
