//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QStack>
#include <QThread>
#include <QFutureWatcher>
#include <QSystemTrayIcon>

#include <optional>

#include <widget/widget_shared.h>
#include <base/encodingprofile.h>

#include <widget/uiplayerstateadapter.h>
#include <widget/playlistentity.h>
#include <widget/playerorder.h>
#include <widget/driveinfo.h>
#include <widget/str_utilts.h>
#include <widget/youtubedl/ytmusic.h>
#include <widget/databasecoverid.h>

#include <xampplayer.h>
#include <ui_xamp.h>

class ProcessIndicator;
class YtMusic;
struct MbDiscIdInfo;
struct PlaybackFormat;

class LrcPage;
class PlaylistPage;
class AboutPage;
class PreferencePage;
class AlbumView;
class ArtistView;
class AlbumArtistPage;
class ArtistInfoPage;
class PlaybackHistoryPage;
class QWidgetAction;
class QFileSystemWatcher;
class FileSystemViewPage;
class QRadioButton;
class BackgroundWorker;
class CdPage;
class XMenu;
class DatabaseFacade;
class XMessage;
class XProgressDialog;
class FindAlbumCoverWorker;
class FileSystemWorker;
class PlaylistTabWidget;
class PlayListTableView;
class GenreViewPage;

class Xamp final : public IXFrame {
	Q_OBJECT

public:
	static constexpr auto kShowProgressDialogMsSecs = 1000;
	static constexpr ConstLatin1String kSoftwareUpdateUrl =
		qTEXT("https://raw.githubusercontent.com/billlin0904/xamp2/master/src/versions/updates.json");

    Xamp(QWidget* parent, const std::shared_ptr<IAudioPlayer> &player);

    virtual ~Xamp() override;

    void setMainWindow(IXMainWindow* main_window);

    void setThemeColor(QColor background_color, QColor color);

	void shortcutsPressed(const QKeySequence& shortcut) override;

	void setFullScreen();

	void initialDeviceList();

	void waitForReady();

signals:
	void payNextMusic();

    void themeChanged(QColor background_color, QColor color);

	void currentThemeChanged(ThemeColor theme_color);

	void blurImage(const QString& cover_id, const QPixmap& image, QSize size);

	void fetchCdInfo(const DriveInfo& drive);

	void searchLyrics(int32_t music_id, const QString& title, const QString& artist);

	void extractFile(const QString& file_path, int32_t playlist_id, bool is_podcast_mode);	

	void translation(const QString& keyword, const QString& from, const QString& to);	

	void changePlayerOrder(PlayerOrder order);

	void updateNewVersion(const Version &version);

	void fetchThumbnailUrl(const DatabaseCoverId &id, const QString& thumbnail_url);

	void setWatchDirectory(const QString& dir);

public slots:
	void onDelayedDownloadThumbnail();

    void onPlayEntity(const PlayListEntity& entity);

	void ensureLocalOnePlaylistPage();

	void onPlayMusic(const PlayListEntity& entity);

    void onAddPlaylistItem(const QList<int32_t>& music_ids, const QList<PlayListEntity>& entities);

	void onArtistIdChanged(const QString& artist, const QString& cover_id, int32_t artist_id);

	void onActivated(QSystemTrayIcon::ActivationReason reason);

	void onVolumeChanged(float volume);

	void onSetCover(const QString& cover_id, PlaylistPage* page);

	void onUpdateCdTrackInfo(const QString& disc_id, const ForwardList<TrackInfo>& track_infos);

	void onUpdateMbDiscInfo(const MbDiscIdInfo& mb_disc_id_info);

	void onUpdateDiscCover(const QString& disc_id, const QString& cover_id);

	void onSearchLyricsCompleted(int32_t music_id, const QString& lyrics, const QString& trlyrics);

	void onSearchArtistCompleted(const QString& artist, const QByteArray& image);

	void onThemeChangedFinished(ThemeColor theme_color);

	void onInsertDatabase(const ForwardList<TrackInfo>& result, int32_t playlist_id);

	void onReadFileProgress(int32_t progress);

	void onReadCompleted();

	void onReadFileStart();

	void onReadFilePath(const QString& file_path);

	void onFoundFileCount(size_t file_count);

	void onSetAlbumCover(int32_t album_id, const QString& cover_id);

	void onTranslationCompleted(const QString& keyword, const QString& result);

	void onEditTags(int32_t playlist_id, const QList<PlayListEntity>& entities);

	void onCheckForUpdate();

	void onRestartApp();

	void onSearchCompleted(const std::vector<search::SearchResultItem>& result);

	void onFetchAlbumCompleted(const album::Album& album);

	void onFetchPlaylistTrackCompleted(PlaylistPage* playlist_page, const std::vector<playlist::Track>& tracks);

	void onSearchSuggestionsCompleted(const std::vector<std::string>& result);

	void onSetThumbnail(const DatabaseCoverId& id, const QString& cover_id);

	void onFetchThumbnailUrlError(const DatabaseCoverId& id, const QString& thumbnail_url);
private:
	void initialUi();

	void initialPlaylist();

	void initialCloudPlaylist();

	void initialController();

	void initialShortcut();

	void initialSpectrum();

	void destroy();

	void drivesChanges(const QList<DriveInfo>& drive_infos) override;

	void drivesRemoved(const DriveInfo& drive_info) override;

    void stopPlay() override;

    void playNext() override;

    void playPrevious() override;

    void playOrPause() override;

    void addDropFileItem(const QUrl& url) override;

	void closeEvent(QCloseEvent* event) override;

	void updateMaximumState(bool is_maximum) override;

	void setPlaylistPageCover(const QPixmap* cover, PlaylistPage* page = nullptr);

	QWidgetAction* createDeviceMenuWidget(const QString& desc, const QIcon& icon = QIcon());

	void onSampleTimeChanged(double stream_time);

	void playLocalFile(const PlayListEntity& entity);

	void onPlayerStateChanged(PlayerState play_state);

	void addItem(const QString& file_name);

    void setVolume(uint32_t volume);

	void setCurrentTab(int32_t table_id);

	void playNextItem(int32_t forward);

	void setPlayerOrder(bool emit_order = false);

	void pushWidget(QWidget* widget);

	void setSeekPosValue(double stream_time_as_ms);

	void resetSeekPosValue();

    void onDeviceStateChanged(DeviceState state);

    void encodeFlacFile(const PlayListEntity& entity);

	void encodeAacFile(const PlayListEntity& entity, const EncodingProfile & profile);

	void encodeWavFile(const PlayListEntity& entity);

	void downloadFile(const PlayListEntity& entity);

    void updateUi(const PlayListEntity& entity, const PlaybackFormat& playback_format, bool open_done);

	void updateButtonState();

    void setupDsp(const PlayListEntity& item) const;

	void connectPlaylistPageSignal(PlaylistPage* playlist_page);

	void appendToPlaylist(const QString& file_name, bool append_to_playlist);

	void setupSampleWriter(ByteFormat byte_format, PlaybackFormat& playback_format) const;

	void setupSampleRateConverter(std::function<void()>& initial_sample_rate_converter,
		uint32_t& target_sample_rate,
		QString& sample_rate_converter_type) const;
	
	PlaylistPage* newPlaylistPage(PlaylistTabWidget* tab_widget, int32_t playlist_id, const QString& cloud_playlist_id, const QString &name);

	[[nodiscard]] PlaylistPage* localPlaylistPage() const;

	void playCloudVideoId(const PlayListEntity& entity, const QString& video_id);

	void fetchLyrics(const PlayListEntity& entity, const QString& video_id);

	QString translateDeviceDescription(const IDeviceType* device_type);

	QString translateError(Errors error);

	bool is_seeking_;
	bool trigger_upgrade_action_;
	bool trigger_upgrade_restart_;
	PlayerOrder order_;
	QModelIndex play_index_;
	IXMainWindow* main_window_;
	PlayListTableView* last_play_list_{ nullptr };
	PlaylistPage* last_play_page_{ nullptr };
	std::optional<DeviceInfo> device_info_;
	std::optional<PlayListEntity> current_entity_;
	QScopedPointer<PlaylistTabWidget> local_tab_widget_;
	QScopedPointer<LrcPage> lrc_page_;	
	QScopedPointer<PlaylistPage> music_page_;
	QScopedPointer<CdPage> cd_page_;	
	QScopedPointer<AlbumArtistPage> album_page_;
	QScopedPointer<FileSystemViewPage> file_system_view_page_;
	QScopedPointer<PlaylistPage> cloud_search_page_;
	QScopedPointer<PlaylistTabWidget> cloud_tab_widget_;
	QScopedPointer<BackgroundWorker> background_worker_;
	QScopedPointer<FindAlbumCoverWorker> find_album_cover_worker_;
	QScopedPointer<FileSystemWorker> extract_file_worker_;
	QScopedPointer<YtMusic> ytmusic_worker_;
    QThread background_thread_;
	QThread find_album_cover_thread_;
	QThread file_system_thread_;
	QThread ytmusic_thread_;
	QTimer ui_update_timer_timer_;
	QMap<DatabaseCoverId, QString> download_thumbnail_pending_;
	std::shared_ptr<UIPlayerStateAdapter> state_adapter_;
	std::shared_ptr<IAudioPlayer> player_;
	QVector<QFrame*> device_type_frame_;
	QSharedPointer<XProgressDialog> read_progress_dialog_;
	QElapsedTimer progress_timer_;
    Ui::XampWindow ui_;	
};
