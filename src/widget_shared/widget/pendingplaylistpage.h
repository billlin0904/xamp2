//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QTableView>

#include <widget/widget_shared_global.h>

class QSqlQueryModel;

class XAMP_WIDGET_SHARED_EXPORT PendingPlayTableView : public QTableView {
	Q_OBJECT
public:
	explicit PendingPlayTableView(QWidget* parent = nullptr);

	void Reload();

	void SetPlaylistId(int32_t playlist_id);

	QModelIndex GetHoverIndex() const {
		return model()->index(hover_row_, hover_column_);
	}

	void mouseMoveEvent(QMouseEvent* event) override;
private:
	int32_t hover_row_{ -1 };
	int32_t hover_column_{ -1 };
	int32_t playlist_id_{ -1 };
	QSqlQueryModel* model_;
};

class XAMP_WIDGET_SHARED_EXPORT PendingPlaylistPage : public QFrame {
	Q_OBJECT
public:
	PendingPlaylistPage(const QList<QModelIndex>& indexes, int32_t playlist_id, QWidget* parent);

	PendingPlayTableView* playlist() {
		return playlist_;
	}
signals:
	void PlayMusic(const QModelIndex& index);

private:
	PendingPlayTableView* playlist_;
	QList<QModelIndex> indexes_;
};
