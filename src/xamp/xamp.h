//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QStack>
#include <QThread>
#include <QVector>
#include <QSystemTrayIcon>

#include <optional>

#include <widget/widget_shared.h>
#include <base/encodingprofile.h>

#include <widget/uiplayerstateadapter.h>
#include <widget/playlistentity.h>
#include <widget/playerorder.h>
#include <widget/driveinfo.h>
#include <widget/str_utilts.h>

#include <xampplayer.h>
#include <ui_xamp.h>

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
class ExtractFileWorker;

class Xamp final : public IXFrame {
	Q_OBJECT

public:
	inline static const QString kSoftwareUpdateUrl =
		qTEXT("https://raw.githubusercontent.com/billlin0904/xamp2/master/src/versions/updates.json");

    Xamp(QWidget* parent, const std::shared_ptr<IAudioPlayer> &player);

    virtual ~Xamp() override;

    void SetXWindow(IXMainWindow* main_window);

    void SetThemeColor(QColor background_color, QColor color);

	void ShortcutsPressed(const QKeySequence& shortcut) override;

	void SetFullScreen();

	void InitialDeviceList();

	void WaitForReady();
signals:
	void PayNextMusic();

    void ThemeChanged(QColor background_color, QColor color);

	void NowPlaying(QString const& artist, QString const& title);

	void BlurImage(const QString& cover_id, const QPixmap& image, QSize size);

	void FetchCdInfo(const DriveInfo& drive);

	void SearchLyrics(int32_t music_id, const QString& title, const QString& artist);

	void ExtractFile(const QString& file_path, int32_t playlist_id, bool is_podcast_mode);	

	void Translation(const QString& keyword, const QString& from, const QString& to);	

	void ChangePlayerOrder(PlayerOrder order);
public slots:
    void PlayEntity(const PlayListEntity& entity);

	void PlayPlayListEntity(const PlayListEntity& entity);

    void AddPlaylistItem(const QList<int32_t>& music_ids, const QList<PlayListEntity>& entities);

	void OnArtistIdChanged(const QString& artist, const QString& cover_id, int32_t artist_id);

	void ProcessTrackInfo(int32_t total_album, int32_t total_tracks) const;

	void OnActivated(QSystemTrayIcon::ActivationReason reason);

	void OnVolumeChanged(float volume);

	void SetCover(const QString& cover_id, PlaylistPage* page);

	void OnUpdateCdTrackInfo(const QString& disc_id, const ForwardList<TrackInfo>& track_infos);

	void OnUpdateMbDiscInfo(const MbDiscIdInfo& mb_disc_id_info);

	void OnUpdateDiscCover(const QString& disc_id, const QString& cover_id);

	void OnSearchLyricsCompleted(int32_t music_id, const QString& lyrics, const QString& trlyrics);

	void OnSearchArtistCompleted(const QString& artist, const QByteArray& image);

	void OnCurrentThemeChanged(ThemeColor theme_color);

	void OnInsertDatabase(const ForwardList<TrackInfo>& result, int32_t playlist_id);

	void OnReadFileProgress(int32_t progress);

	void OnReadCompleted();

	void OnReadFileStart();

	void OnReadFilePath(const QString& file_path);

	void OnFoundFileCount(size_t file_count);

	void OnSetAlbumCover(int32_t album_id,
		const QString& album,
		const QString& cover_id);

	void OnTranslationCompleted(const QString& keyword, const QString& result);

private:
	void DrivesChanges(const QList<DriveInfo>& drive_infos) override;

	void DrivesRemoved(const DriveInfo& drive_info) override;

    void StopPlay() override;

    void PlayNext() override;

    void PlayPrevious() override;

    void PlayOrPause() override;

    void DeleteKeyPress() override;

    void AddDropFileItem(const QUrl& url) override;

	void closeEvent(QCloseEvent* event) override;

	void UpdateMaximumState(bool is_maximum) override;

    void FocusIn() override;

    void FocusOut() override;

	void SetPlaylistPageCover(const QPixmap* cover, PlaylistPage* page = nullptr);

	QWidgetAction* CreateDeviceMenuWidget(const QString& desc, const QIcon& icon = QIcon());

	void OnSampleTimeChanged(double stream_time);

	void PlayLocalFile(const PlayListEntity& item);

	void OnPlayerStateChanged(PlayerState play_state);

	void AddItem(const QString& file_name);

    void SetVolume(uint32_t volume);

	void SetCurrentTab(int32_t table_id);

	void InitialUi();

	void InitialPlaylist();

	void InitialController();

	void InitialShortcut();

	void InitialSpectrum();

	void PlayNextItem(int32_t forward);

    void SetTablePlaylistView(int table_id, ConstLatin1String column_setting_name);

	void SetPlayerOrder(bool emit_order = false);

	PlaylistPage* NewPlaylistPage(int32_t playlist_id, const QString& column_setting_name);

	void PushWidget(QWidget* widget);

	QWidget* PopWidget();

	QWidget* TopWidget();

	void GoBackPage();

	void GetNextPage();

	void SetSeekPosValue(double stream_time_as_ms);

	void ResetSeekPosValue();

    void OnDeviceStateChanged(DeviceState state);

    void EncodeFlacFile(const PlayListEntity& item);

	void EncodeAacFile(const PlayListEntity& item, const EncodingProfile & profile);

	void EncodeWavFile(const PlayListEntity& item);

    void UpdateUi(const PlayListEntity& item, const PlaybackFormat& playback_format, bool open_done);

	void UpdateButtonState();
	
	PlaylistPage* CurrentPlaylistPage();

	void cleanup();

    void SetupDsp(const PlayListEntity& item) const;

	void ConnectPlaylistPageSignal(PlaylistPage* playlist_page);

	void AppendToPlaylist(const QString& file_name);

	QString TranslateErrorCode(const Errors error) const;

	void SetupSampleWriter(ByteFormat byte_format,
		PlaybackFormat& playback_format) const;

	void SetupSampleRateConverter(std::function<void()>& initial_sample_rate_converter,
		uint32_t& target_sample_rate,
		QString& sample_rate_converter_type);

	void showEvent(QShowEvent* event) override;

	bool is_seeking_;
	bool trigger_upgrade_action_;
	PlayerOrder order_;
	QModelIndex play_index_;
	IXMainWindow* main_window_;
	PlaylistPage* current_playlist_page_;
	std::optional<DeviceInfo> device_info_;
	std::optional<PlayListEntity> current_entity_;
	QScopedPointer<LrcPage> lrc_page_;
	QScopedPointer<PlaylistPage> playlist_page_;
	QScopedPointer<PlaylistPage> music_page_;
	QScopedPointer<CdPage> cd_page_;
	QScopedPointer<AlbumArtistPage> album_page_;
	QScopedPointer<FileSystemViewPage> file_system_view_page_;
	QScopedPointer<BackgroundWorker> background_worker_;
	QScopedPointer<FindAlbumCoverWorker> find_album_cover_worker_;
	QScopedPointer<ExtractFileWorker> extract_file_worker_;
    QStack<int32_t> stack_page_id_;
    QThread background_thread_;
	QThread find_album_cover_thread_;
	QThread extract_file_thread_;
	std::shared_ptr<UIPlayerStateAdapter> state_adapter_;
	std::shared_ptr<IAudioPlayer> player_;
	QVector<QFrame*> device_type_frame_;
	QSharedPointer<XProgressDialog> read_progress_dialog_;
	QElapsedTimer progress_timer_;
    Ui::XampWindow ui_;
};
