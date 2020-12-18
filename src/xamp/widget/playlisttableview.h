//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <optional>

#include <QTableView>
#include <QKeyEvent>
#include <QSqlTableModel>
#include <QSqlQueryModel>

#include <widget/widget_shared.h>

#include <widget/playlisttablemodel.h>
#include <widget/playlistentity.h>
#include <widget/playlisttableproxymodel.h>
#include <widget/playlistsqlquerytablemodel.h>
#include <widget/metadataextractadapter.h>

class StarDelegate;

class PlayListTableView final : public QTableView {
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

    void setCurrentPlayIndex(const QModelIndex& index);

	QModelIndex nextIndex(int forward) const;

	std::optional<QModelIndex> selectItem() const;

	void play(const QModelIndex& index);

    void scrollToIndex(const QModelIndex& index);

	void resizeColumn() const;

	std::map<int32_t, QModelIndex> selectItemIndex() const;

	void append(const QString& file_name);

	static PlayListEntity fromMetadata(const Metadata& metadata);

    void refresh();

    PlayListEntity nomapItem(const QModelIndex& index);

signals:
	void removeItems(int32_t playlist_id, const QVector<int>& select_music_ids);

	void playMusic(const QModelIndex& index, const PlayListEntity& item);

	void readFingerprint(const QModelIndex& index, const PlayListEntity& item);

	void setLoopTime(double start_time, double end_time);

public slots:
	void processMeatadata(const std::vector<Metadata> &medata);

	void search(const QString& sort_str, Qt::CaseSensitivity case_sensitivity, QRegExp::PatternSyntax pattern_syntax);

	void onTextColorChanged(QColor backgroundColor, QColor color);

private:
    PlayListEntity item(const QModelIndex& index);

	bool eventFilter(QObject* obj, QEvent* ev) override;

    void keyPressEvent(QKeyEvent *pEvent) override;

	void reloadSelectMetadata();

    void resizeEvent(QResizeEvent* event) override;

	void initial();

	int32_t playlist_id_;
	StarDelegate* start_delegate_;
	QModelIndex play_index_;
    PlayListSqlQueryTableModel model_;
	PlayListTableFilterProxyModel proxy_model_;
};


