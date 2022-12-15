//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QTableView>

#include <optional>

#include <widget/playlistentity.h>
#include <widget/database.h>
#include <widget/metadataextractadapter.h>

#include <base/rng.h>
#include <base/encodingprofile.h>

class StarDelegate;
class ProcessIndicator;
class PlayListSqlQueryTableModel;
class PlayListTableFilterProxyModel;

class PlayListTableView : public QTableView {
	Q_OBJECT
public:
	explicit PlayListTableView(QWidget* parent = nullptr, int32_t playlist_id = 1);

	virtual ~PlayListTableView() override;

	virtual void reload();

	void executeQuery();

	void setPlaylistId(const int32_t playlist_id, const QString& column_setting_name);

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

	void setNowPlayState(PlayingState playing_state);

    void scrollToIndex(const QModelIndex& index);

	void resizeColumn();

	std::map<int32_t, QModelIndex> selectItemIndex() const;

	void append(const QString& file_name, bool show_progress_dialog = true, bool is_recursive = true);

	QModelIndex hoverIndex() const {
		return model()->index(hover_row_, hover_column_);
	}
signals:
	void playMusic(const PlayListEntity& item);

    void encodeFlacFile(const PlayListEntity& item);

	void encodeAACFile(const PlayListEntity& item, const EncodingProfile& profile);

	void encodeWavFile(const PlayListEntity& item);

    void addPlaylistReplayGain(bool force, const ForwardList<PlayListEntity> &entities);

	void updateAlbumCover(const QString &cover_id);

	void addPlaylistItemFinished();

	void downloadPodcast();

public slots:
	void processDatabase(const ForwardList<PlayListEntity>& entities);

	void processMeatadata(int64_t dir_last_write_time, const ForwardList<TrackInfo> &medata);

	void search(const QString& sort_str, Qt::CaseSensitivity case_sensitivity, QRegExp::PatternSyntax pattern_syntax);

	void onThemeColorChanged(QColor backgroundColor, QColor color);

	void updateReplayGain(const PlayListEntity& entity,
		double track_loudness,
		double album_rg_gain,
		double album_peak,
		double track_rg_gain,
		double track_peak);

	void onDownloadPodcastCompleted(const ForwardList<TrackInfo>& track_infos, const QByteArray& cover_image_data);
private:
	PlayListEntity item(const QModelIndex& index);

	void playItem(const QModelIndex& index);

	void pauseItem(const QModelIndex& index);

	bool eventFilter(QObject* obj, QEvent* ev) override;

    void keyPressEvent(QKeyEvent *pEvent) override;

    void resizeEvent(QResizeEvent* event) override;

	void mouseMoveEvent(QMouseEvent* event) override;

	void initial();

protected:
	bool podcast_mode_;
	int32_t hover_row_{ -1 };
	int32_t hover_column_{ -1 };
	int32_t playlist_id_;
	QModelIndex play_index_;
	StarDelegate* start_delegate_;	
    PlayListSqlQueryTableModel* model_;
	PlayListTableFilterProxyModel* proxy_model_;
    QSet<QString> notshow_column_names_;
	PRNG rng_;
	QString column_setting_name_;
	QSharedPointer<ProcessIndicator> indicator_;
};


