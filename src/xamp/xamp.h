//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QSystemTrayIcon>
#include <QThread>
#include <optional>

#include <widget/widget_shared.h>

#include <widget/uiplayerstateadapter.h>
#include <widget/playlistentity.h>
#include <widget/playerorder.h>
#include <widget/driveinfo.h>
#include <widget/util/str_util.h>
#include <widget/databasecoverid.h>

#include <widget/dao/dbfacade.h>
#include <widget/encodejobwidget.h>
#include <widget/httpx.h>

#include <xampplayer.h>
#include <ui_xamp.h>

class PlaylistTabPage;
class ProcessIndicator;
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

	void searchLyrics(const PlayListEntity& keyword);

	void extractFile(const QString& file_path, int32_t playlist_id, bool is_podcast_mode);	

	void changePlayerOrder(PlayerOrder order);

	void updateNewVersion(const QVersionNumber&version);

	void fetchThumbnailUrl(const DatabaseCoverId &id, const QString& thumbnail_url);

	void findAlbumCover(const DatabaseCoverId& id);

	void cancelRequested();

	void addJobs(const QString& dir_name, QList<EncodeJob> jobs);

	void transcribeFile(const QString& file_name);
public slots:
    void onPlayEntity(const PlayListEntity& entity, bool is_double_clicked);

	void ensureLocalOnePlaylistPage();

	void onPlayMusic(int32_t playlist_id, const PlayListEntity& entity, bool is_double_clicked);

    void onAddPlaylist(int32_t playlist_id, const QList<int32_t>& music_ids);

	void onArtistIdChanged(const QString& artist, const QString& cover_id, int32_t artist_id);	

	void onSetCover(const QString& cover_id, PlaylistPage* page);

	void onUpdateCdTrackInfo(const QString& disc_id, const std::forward_list<TrackInfo>& track_infos);

	void onUpdateMbDiscInfo(const MbDiscIdInfo& mb_disc_id_info);

	void onUpdateDiscCover(const QString& disc_id, const QString& cover_id);

	void onSearchArtistCompleted(const QString& artist, const QByteArray& image);

	void onThemeChangedFinished(ThemeColor theme_color);

	void onInsertDatabase(const std::forward_list<TrackInfo>& result, int32_t playlist_id);

	void onBatchInsertDatabase(const std::vector<std::forward_list<TrackInfo>>& results, int32_t playlist_id);

	void onSetAlbumCover(int32_t album_id, const QString& cover_id);

	void onEditTags(int32_t playlist_id, const QList<PlayListEntity>& entities);

	void onCheckForUpdate();

	void onRestartApp();

	void onSetThumbnail(const DatabaseCoverId& id, const QString& cover_id);

	void onPlaybackError(const QString& message);

	void onRetranslateUi();

	void onPlayerStateChanged(PlayerState play_state);

	void onDeviceStateChanged(DeviceState state, const QString& device_id);

	void onSampleTimeChanged(double stream_time);

	void onActivated(QSystemTrayIcon::ActivationReason reason);

	void onEncodeAlacFiles(int32_t encode_type, const QList<PlayListEntity>& files);

	void onCancelRequested();

	void onFetchMbDiscInfoCompleted(const MbDiscIdInfo& mb_disc_id_info);
private:
	void initialUi();

	void initialPlaylist();

	void initialCloudPlaylist();

	void initialController();

	void showNaviBarButton();

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

    void updateUi(const PlayListEntity& entity,
		const PlaybackFormat& playback_format, 
		bool open_done, bool is_double_clicked);

    void setupDsp(const PlayListEntity& item) const;

	void connectPlaylistPageSignal(PlaylistPage* playlist_page);

	void appendToPlaylist(const QString& file_name, bool append_to_playlist);

	void appendToPlaylistId(const QString& file_name, int32_t playlist_id);

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
	QScopedPointer<AlbumArtistPage> library_page_;
	QScopedPointer<FileSystemViewPage> file_explorer_page_;
	QScopedPointer<PlaylistTabPage> playlist_tab_page_;
	QScopedPointer<BackgroundService> background_service_;
	QScopedPointer<AlbumCoverService> album_cover_service_;
	QScopedPointer<FileSystemService> file_system_service_;
	QScopedPointer<QSystemTrayIcon> tray_icon_;
	QList<QWidget*> widgets_;
    QThread background_service_thread_;
	QThread album_cover_service_thread_;
	QThread file_system_service_thread_;
	QMap<DatabaseCoverId, QString> download_thumbnail_pending_;
	QList<QFrame*> device_type_frame_;
	http::HttpClient http_client_;
	std::shared_ptr<UIPlayerStateAdapter> state_adapter_;
	std::shared_ptr<IAudioPlayer> player_;
	std::shared_ptr<IThreadPoolExecutor> thread_pool_;
	Ui::XampWindow ui_;
};
