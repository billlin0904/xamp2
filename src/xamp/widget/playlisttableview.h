//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <optional>

#include <QTableView>

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

	void setPodcastMode(bool enable = true);

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

	void resizeColumn();

	std::map<int32_t, QModelIndex> selectItemIndex() const;

	void append(const QString& file_name, bool show_progress_dialog = true);

    void refresh();

    PlayListEntity nomapItem(const QModelIndex& index);

signals:
	void removeItems(int32_t playlist_id, const QVector<int>& select_music_ids);

	void playMusic(const QModelIndex& index, const PlayListEntity& item);

	void readFileLUFS(const PlayListEntity& item);

    void exportWaveFile(const PlayListEntity& item);

    void encodeFlacFile(const PlayListEntity& item);

public slots:
	void processMeatadata(const std::vector<Metadata> &medata);

	void search(const QString& sort_str, Qt::CaseSensitivity case_sensitivity, QRegExp::PatternSyntax pattern_syntax);

	void onThemeColorChanged(QColor backgroundColor, QColor color);

	void onReadCompleted(int32_t music_id, double lrus, double trure_peak);

private:
    PlayListEntity item(const QModelIndex& index);

	bool eventFilter(QObject* obj, QEvent* ev) override;

    void keyPressEvent(QKeyEvent *pEvent) override;

    void resizeEvent(QResizeEvent* event) override;

	void importPodcast();

	void initial();

	bool podcast_mode_;
	int32_t playlist_id_;
	StarDelegate* start_delegate_;	
	QModelIndex play_index_;
    PlayListSqlQueryTableModel model_;
	PlayListTableFilterProxyModel proxy_model_;
};


