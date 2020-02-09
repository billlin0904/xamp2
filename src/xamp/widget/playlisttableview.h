//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <optional>

#include <QTableView>
#include <QKeyEvent>

#include <base/metadata.h>

#include <widget/playlisttablemodel.h>
#include <widget/playlistentity.h>
#include <widget/playlisttableproxymodel.h>
#include <widget/metadataextractadapter.h>

class PlayListTableView : public QTableView {
	Q_OBJECT
public:
	explicit PlayListTableView(QWidget* parent = nullptr, int32_t playlist_id = 1);

	~PlayListTableView() override;

	void setPlaylistId(int32_t playlist_id);

	int32_t playlistId() const;

	void removePlaying();

	void removeItem(const QModelIndex& index);

	void removeSelectItems();

	void setNowPlaying(const QModelIndex& index, bool is_scroll_to = false);

	QModelIndex shuffeIndex();

	QModelIndex currentIndex() const;

	PlayListEntity& item(const QModelIndex& index);

	std::optional<QModelIndex> selectItem() const;

	void play(const QModelIndex& index);

    void scrollTo(const QModelIndex& index);

	void resizeColumn() const;

	std::map<int32_t, QModelIndex> selectItemIndex() const;

	void append(const QString& file_name, bool add_playlist);

	static PlayListEntity fromMetadata(const xamp::base::Metadata& metadata);

signals:
	void removeItems(int32_t playlist_id, const QVector<int>& select_music_ids);

	void playMusic(const QModelIndex& index, const PlayListEntity& item);

	void readFingerprint(const QModelIndex& index, const PlayListEntity& item);

public slots:
	void appendItem(const xamp::base::Metadata& metadata);

	void appendItem(const PlayListEntity& item);

	void search(const QString& sort_str, Qt::CaseSensitivity case_sensitivity, QRegExp::PatternSyntax pattern_syntax);

private:
	bool eventFilter(QObject* obj, QEvent* ev) override;

	void reloadSelectMetadata();

    void resizeEvent(QResizeEvent* event) override;

	void initial();

	int32_t playlist_id_;
	QModelIndex play_index_;
	PlayListTableModel model_;
	PlayListTableFilterProxyModel proxy_model_;
	MetadataExtractAdapter adapter_;
};


