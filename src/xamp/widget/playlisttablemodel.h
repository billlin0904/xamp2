//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>
#include <widget/playlistentity.h>

#include <QStandardItemModel>

enum PlayListColumn {
	PLAYLIST_MUSIC_ID = 0,
	PLAYLIST_PLAYING,
	PLAYLIST_TRACK,
	PLAYLIST_FILEPATH,
	PLAYLIST_TITLE,
	PLAYLIST_FILE_NAME,
	PLAYLIST_ARTIST,
	PLAYLIST_ALBUM,
	PLAYLIST_DURATION,
	PLAYLIST_BITRATE,
	PLAYLIST_SAMPLE_RATE,
	PLAYLIST_RATING,
	_PLAYLIST_MAX_COLUMN_,
};

class PlayListTableModel final : public QAbstractTableModel {
	Q_OBJECT
public:
	explicit PlayListTableModel(QObject* parent = nullptr);

	~PlayListTableModel() override = default;

	int32_t rowCount(const QModelIndex& parent) const override;

	int32_t columnCount(const QModelIndex& parent) const override;

	QVariant data(const QModelIndex& index, int32_t role = Qt::DisplayRole) const override;

	QVariant headerData(int32_t section, Qt::Orientation orientation, int32_t role = Qt::DisplayRole) const override;

	bool removeRows(int32_t position, int32_t rows, const QModelIndex& parent = QModelIndex()) override;

	bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

	Qt::ItemFlags flags(const QModelIndex& index) const override;

	void append(const PlayListEntity& item);

	PlayListEntity& item(const QModelIndex& index);

	void removeRow(int32_t pos);

	bool isEmpty() const noexcept;

	void clear();

	void setNowPlaying(int32_t playing_index) noexcept;

	int32_t nowPlaying() const noexcept;

	PlayListEntity& item(int32_t index);

private:
	int32_t playing_index_;
	QList<PlayListEntity> data_;
};
