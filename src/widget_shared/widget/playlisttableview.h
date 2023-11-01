//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QTableView>

#include <optional>

#include <widget/playlistentity.h>
#include <widget/database.h>
#include <widget/databasefacade.h>
#include <widget/playerorder.h>
#include <widget/widget_shared_global.h>

#include <base/rng.h>
#include <base/encodingprofile.h>

class PlayListSqlQueryTableModel;
class PlayListTableFilterProxyModel;

class XAMP_WIDGET_SHARED_EXPORT PlayListTableView : public QTableView {
	Q_OBJECT
public:
	static constexpr auto kMinimalEncodingBitRate = 128;
    static constexpr auto kMaxStretchedSize = 500;
	static constexpr auto kMaxPendingPlayListSize = 100;

	static constexpr auto kColumnPlayingWidth = 25;
	static constexpr auto kColumnTrackWidth = 40;
	static constexpr auto kColumnArtistWidth = 300;
	static constexpr auto kColumnCoverIdWidth = 42;
	static constexpr auto kColumnDefaultWidth = 120;
	static constexpr auto kColumnDurationWidth = 10;
	static constexpr auto kColumnHeight = 46;

	static constexpr auto kPendingPlaylistSize = 30;

	explicit PlayListTableView(QWidget* parent = nullptr, int32_t playlist_id = 1);

	~PlayListTableView() override;

	virtual void FastReload();

	void DisableDelete(bool enable = false) {
		enable_delete_ = enable;
	}

	void DisableLoadFile(bool enable = false) {
		enable_load_file_ = enable;
	}

	void SetPlaylistId(const int32_t playlist_id, const QString& column_setting_name);

	int32_t GetPlaylistId() const;

	void Reload();

	void RemovePlaying();

	void RemoveAll();

	void RemoveItem(const QModelIndex& index);

	void RemoveSelectItems();

	void SetNowPlaying(const QModelIndex& index, bool is_scroll_to = false);

	QModelIndex GetFirstIndex() const;

	QModelIndex GetShuffleIndex();

	QModelIndex GetCurrentIndex() const;
    
    void DeletePendingPlaylist();

    void SetCurrentPlayIndex(const QModelIndex& index);

	QModelIndex GetNextIndex(int forward) const;

	std::optional<QModelIndex> GetSelectItem() const;

	std::optional<PlayListEntity> GetSelectPlayListEntity() const;

    void Play(PlayerOrder order);

	void SetNowPlayState(PlayingState playing_state);

    void ScrollToIndex(const QModelIndex& index);

	void ResizeColumn();

	OrderedMap<int32_t, QModelIndex> SelectItemIndex() const;

	void append(const QString& file_name);

	QModelIndex GetHoverIndex() const {
		return model()->index(hover_row_, hover_column_);
	}

	void SetHeaderViewHidden(bool enable);

	QList<QModelIndex> GetPendingPlayIndexes() const;

	void SetOtherPlaylist(int32_t playlist_id);

	void Search(const QString& keyword) const;
signals:
	void UpdatePlayingState(const PlayListEntity &entity, PlayingState playing_state);

	void PlayMusic(const PlayListEntity& item);

    void EncodeFlacFile(const PlayListEntity& item);

	void EncodeAacFile(const PlayListEntity& item, const EncodingProfile& profile);

	void EncodeWavFile(const PlayListEntity& item);

    void ReadReplayGain(int32_t playlist_id, const QList<PlayListEntity> &entities);

	void EditTags(int32_t playlist_id, const QList<PlayListEntity>& entities);

	void UpdateAlbumCover(const QString &cover_id);

	void AddPlaylistItemFinished();

	void ExtractFile(const QString& file_path, int32_t playlist_id);
public slots:
	void PlayIndex(const QModelIndex& index);

	void ProcessDatabase(int32_t playlist_id, const QList<PlayListEntity>& entities);

	void ProcessTrackInfo(int32_t total_album, int32_t total_tracks);

	void OnThemeColorChanged(QColor backgroundColor, QColor color);

	void ReloadEntity(const PlayListEntity& item);

	void UpdateReplayGain(int32_t playlistId, 
		const PlayListEntity& entity,
		double track_loudness,
		double album_rg_gain,
		double album_peak,
		double track_rg_gain,
		double track_peak);

	void AddPendingPlayListFromModel(PlayerOrder order);
private:
	PlayListEntity item(const QModelIndex& index) const;

	void PlayItem(const QModelIndex& index);

	void PauseItem(const QModelIndex& index);

	bool eventFilter(QObject* obj, QEvent* ev) override;

    void keyPressEvent(QKeyEvent *event) override;

    void resizeEvent(QResizeEvent* event) override;

	void mouseMoveEvent(QMouseEvent* event) override;

	void initial();

protected:
	bool enable_delete_{ true };
	bool enable_load_file_{ true };
	int32_t hover_row_{ -1 };
	int32_t hover_column_{ -1 };
	int32_t playlist_id_{ -1 };
	std::optional<int32_t> other_playlist_id_;
	QModelIndex play_index_;
    PlayListSqlQueryTableModel* model_;
	PlayListTableFilterProxyModel* proxy_model_;
    QSet<QString> hidden_column_names_;
	QList<QModelIndex> pending_playlist_;
	PRNG rng_;
	QString column_setting_name_;
};


