//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QSystemTrayIcon>

#include <optional>

#include <widget/widget_shared.h>
#include <base/encodingprofile.h>
#include <widget/youtubedl/ytmusicservice.h>

#include <widget/uiplayerstateadapter.h>
#include <widget/playlistentity.h>
#include <widget/playerorder.h>
#include <widget/driveinfo.h>
#include <widget/util/str_util.h>
#include <widget/databasecoverid.h>

#include <widget/dao/musicdao.h>
#include <widget/dao/albumdao.h>
#include <widget/dao/artistdao.h>
#include <widget/dao/playlistdao.h>
#include <widget/encodejobwidget.h>

#include <xampplayer.h>
#include <ui_xamp.h>

class ProcessIndicator;
class AudioEmbeddingService;
class YtMusicHttpService;
class MusixmatchHttpService;
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
class BackgroundService;
class CdPage;
class XMenu;
class DatabaseFacade;
class XMessage;
class XProgressDialog;
class AlbumCoverService;
class FileSystemService;
class PlaylistTabWidget;
class PlaylistTableView;
class GenreViewPage;
class YtMusicOAuth;
class QScrollArea;
class QSystemTrayIcon;

class Xamp final : public IXFrame {
	Q_OBJECT

public:
    Xamp(QWidget* parent, const std::shared_ptr<IAudioPlayer> &player);

    virtual ~Xamp() override;

    void setMainWindow(IXMainWindow* main_window);

	void shortcutsPressed(const QKeySequence& shortcut) override;

	void initialDeviceList(const std::string& device_id = "");

	QString translateText(const std::string_view& text) override;

signals:
	void payNextMusic();

    void themeColorChanged(QColor background_color, QColor color);

	void blurImage(const QString& cover_id, const QPixmap& image, QSize size);

	void fetchCdInfo(const DriveInfo& drive);

	void searchLyrics(int32_t music_id, const QString& title, const QString& artist);

	void extractFile(const QString& file_path, int32_t playlist_id, bool is_podcast_mode);	

	void changePlayerOrder(PlayerOrder order);

	void updateNewVersion(const QVersionNumber&version);

	void fetchThumbnailUrl(const DatabaseCoverId &id, const QString& thumbnail_url);

	void findAlbumCover(const DatabaseCoverId& id);

	void cancelRequested();

	void addJobs(const QString& dir_name, QList<EncodeJob> jobs);

public slots:
	void onDelayedDownloadThumbnail();

    void onPlayEntity(const PlayListEntity& entity, bool is_doubleclicked, bool is_query_embeddings = false);

	void ensureLocalOnePlaylistPage();

	void onPlayMusic(int32_t playlist_id, const PlayListEntity& entity, bool is_play, bool is_doubleclicked);

    void onAddPlaylist(int32_t playlist_id, const QList<int32_t>& music_ids);

	void onArtistIdChanged(const QString& artist, const QString& cover_id, int32_t artist_id);	

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

	void onEditTags(int32_t playlist_id, const QList<PlayListEntity>& entities);

	void onCheckForUpdate();

	void onRestartApp();

	void onSearchCompleted(const QList<search::Album>& result);

	void onFetchAlbumCompleted(const album::Album& album);

	void onFetchPlaylistTrackCompleted(PlaylistPage* playlist_page, const std::vector<playlist::Track>& tracks);

	void onSearchSuggestionsCompleted(const QList<QString>& result);

	void onSetThumbnail(const DatabaseCoverId& id, const QString& cover_id);

	void onFetchThumbnailUrlError(const DatabaseCoverId& id, const QString& thumbnail_url);

	void onRemainingTimeEstimation(size_t total_work, size_t completed_work, int32_t secs);

	void onPlaybackError(const QString& message);

	void onRetranslateUi();

	void onPlayerStateChanged(PlayerState play_state);

	void onDeviceStateChanged(DeviceState state, const QString& device_id);

	void onCacheYtMusicFile(const PlayListEntity& entity);

	void onSampleTimeChanged(double stream_time);

	void onActivated(QSystemTrayIcon::ActivationReason reason);

	void onEncodeAlacFiles(const QString& codec_id, const QList<PlayListEntity>& files);
private:
	void initialUi();

	void initialPlaylist();

	void initialCloudPlaylist();

	void initialController();

	void showNaviBarButton();

	void initialShortcut();

	void initialSpectrum();

	void destroy();

	void initialYtMusicService();

	void initialAudioEmbeddingService();

	void drivesChanges(const QList<DriveInfo>& drive_infos) override;

	void drivesRemoved(const DriveInfo& drive_info) override;

    void stopPlay() override;

    void playNext() override;

    void playPrevious() override;

    void playOrPause() override;

    void addDropFileItem(const QUrl& url) override;

	void closeEvent(QCloseEvent* event) override;

	void setPlaylistPageCover(const QPixmap* cover, PlaylistPage* page = nullptr);

	QWidgetAction* createDeviceMenuWidget(const QString& desc, const QIcon& icon = QIcon());

	void playLocalFile(const PlayListEntity& entity);

	void addItem(const QString& file_name);

    void setVolume(uint32_t volume);

	void setCurrentTab(int32_t table_id);

	void playNextItem(int32_t forward, bool is_play = true);

	void setPlayerOrder(bool emit_order = false);

	void pushWidget(QWidget* widget);

	void setSeekPosValue(double stream_time_as_ms);

	void resetSeekPosValue();

    void updateUi(const PlayListEntity& entity, const PlaybackFormat& playback_format, bool open_done, bool is_doubleclicked);

    void setupDsp(const PlayListEntity& item) const;

	void connectPlaylistPageSignal(PlaylistPage* playlist_page);

	void appendToPlaylist(const QString& file_name, bool append_to_playlist);

	void setupSampleWriter(ByteFormat byte_format, PlaybackFormat& playback_format) const;

	void setupSampleRateConverter(std::function<void()>& initial_sample_rate_converter,
		uint32_t& target_sample_rate,
		QString& sample_rate_converter_type) const;
	
	PlaylistPage* newPlaylistPage(PlaylistTabWidget* tab_widget,
		int32_t playlist_id, 
		const QString& cloud_playlist_id, 
		const QString &name,
		bool resize = false);

	PlaylistPage* localPlaylistPage() const;

	void playCloudVideoId(const PlayListEntity& entity, const QString& video_id, bool is_doubleclicked);

	QString translateDeviceDescription(const IDeviceType* device_type);

	QString translateError(Errors error);	

	void setCover(const QString& cover_id);

	void showAbout();

	void connectThemeChangedSignal();

	bool is_seeking_;
	bool trigger_upgrade_action_;
	bool trigger_upgrade_restart_;
	int32_t cloud_playlist_process_count_;
	PlayerOrder order_;
	QModelIndex play_index_;
	IXMainWindow* main_window_{ nullptr };
	PlaylistTabWidget* last_playlist_tab_{ nullptr };
	PlaylistTableView* last_playlist_{ nullptr };
    PlaylistPage* last_playlist_page_{ nullptr };
	QAction* preference_action_{ nullptr };
	std::optional<DeviceInfo> device_info_;
	std::optional<PlayListEntity> current_entity_;	
	QScopedPointer<LrcPage> lrc_page_;	
	QScopedPointer<PlaylistPage> music_page_;
	QScopedPointer<CdPage> cd_page_;	
	QScopedPointer<AlbumArtistPage> music_library_page_;
	QScopedPointer<FileSystemViewPage> file_explorer_page_;
	QScopedPointer<PlaylistPage> yt_music_search_page_;
	QScopedPointer<PlaylistTabWidget> playlist_tab_page_;
	QScopedPointer<PlaylistTabWidget> yt_music_tab_page_;
	QScopedPointer<BackgroundService> background_service_;
	QScopedPointer<AlbumCoverService> album_cover_service_;
	QScopedPointer<FileSystemService> file_system_service_;
    QScopedPointer<AudioEmbeddingService> audio_embedding_service_;
	QScopedPointer<MusixmatchHttpService> musixmatch_service_;
	QScopedPointer<YtMusicHttpService> ytmusic_http_service_;
	QScopedPointer<YtMusicOAuth> ytmusic_oauth_;
	QScopedPointer<QSystemTrayIcon> tray_icon_;
	QList<QWidget*> widgets_;
    QThread background_service_thread_;
	QThread album_cover_service_thread_;
	QThread file_system_service_thread_;
	QTimer ui_update_timer_timer_;
	QMap<DatabaseCoverId, QString> download_thumbnail_pending_;
	std::shared_ptr<UIPlayerStateAdapter> state_adapter_;
	std::shared_ptr<IAudioPlayer> player_;
	QVector<QFrame*> device_type_frame_;
	QSharedPointer<XProgressDialog> read_progress_dialog_;
	QElapsedTimer progress_timer_;
	dao::AlbumDao album_dao_;
	dao::ArtistDao artist_dao_;
	dao::MusicDao music_dao_;
	dao::PlaylistDao playlist_dao_;
	http::HttpClient http_client_;
    Ui::XampWindow ui_;
	std::shared_ptr<IThreadPoolExecutor> thread_pool_;
};
