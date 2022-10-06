//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <optional>

#include <QTableView>
#include <base/rng.h>

#include <widget/playlistentity.h>
#include <widget/metadataextractadapter.h>

class StarDelegate;
class PlayListSqlQueryTableModel;
class PlayListTableFilterProxyModel;

class PlayListTableView : public QTableView {
	Q_OBJECT
public:
	explicit PlayListTableView(QWidget* parent = nullptr, int32_t playlist_id = 1);

	virtual ~PlayListTableView() override;

	virtual void refresh();

	void updateData();

	void setPlaylistId(int32_t playlist_id);

	void setPodcastMode(bool enable = true);

	bool isPodcastMode() const;

	int32_t playlistId() const;

	void removePlaying();

	void removeAll();

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

	void append(const QString& file_name, bool show_progress_dialog = true, bool is_recursive = true);

    PlayListEntity nomapItem(const QModelIndex& index);

signals:
	void playMusic(const PlayListEntity& item);

    void encodeFlacFile(const PlayListEntity& item);

	void encodeAACFile(const PlayListEntity& item);

    void addPlaylistReplayGain(bool force, const Vector<PlayListEntity> &entities);

public slots:
	void processMeatadata(int64_t dir_last_write_time, const ForwardList<Metadata> &medata);

	void search(const QString& sort_str, Qt::CaseSensitivity case_sensitivity, QRegExp::PatternSyntax pattern_syntax);

	void onThemeColorChanged(QColor backgroundColor, QColor color);

	void updateReplayGain(int music_id,
		double album_rg_gain,
		double album_peak,
		double track_rg_gain,
		double track_peak);
private:
    PlayListEntity item(const QModelIndex& index);

	bool eventFilter(QObject* obj, QEvent* ev) override;

    void keyPressEvent(QKeyEvent *pEvent) override;

    void resizeEvent(QResizeEvent* event) override;

	void importPodcast();

	void initial();

protected:
	bool podcast_mode_;
	int32_t playlist_id_;
	QModelIndex play_index_;
	StarDelegate* start_delegate_;	
    PlayListSqlQueryTableModel* model_;
	PlayListTableFilterProxyModel* proxy_model_;
    QSet<QString> notshow_column_names_;
	PRNG rng_;
};


