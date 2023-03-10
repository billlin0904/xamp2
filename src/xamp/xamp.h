//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QStack>
#include <QThread>
#include <QVector>
#include <QSystemTrayIcon>

#include <widget/widget_shared.h>

#include <base/encodingprofile.h>
#include <stream/pcm2dsdsamplewriter.h>

#include <widget/uiplayerstateadapter.h>
#include <widget/playlistentity.h>
#include <widget/playerorder.h>
#include <widget/driveinfo.h>
#include <widget/podcast_uiltis.h>

#include "xampplayer.h"
#include "ui_xamp.h"

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
struct PlaybackFormat;
class QRadioButton;
class BackgroundWorker;
class CdPage;
class XMenu;
class DatabaseFacade;
class XMessage;

class Xamp final : public IXFrame {
	Q_OBJECT

public:
    Xamp(QWidget* parent, const std::shared_ptr<IAudioPlayer> &player);

    virtual ~Xamp() override;

    void SetXWindow(IXMainWindow* main_window);

    void SetThemeColor(QColor backgroundColor, QColor color);

	void ShortcutsPressed(const QKeySequence& shortcut) override;

	void SetFullScreen();

signals:
	void PayNextMusic();

    void ThemeChanged(QColor backgroundColor, QColor color);

	void NowPlaying(QString const& artist, QString const& title);

	void BlurImage(const QString& cover_id, const QPixmap& image, QSize size);

	void FetchCdInfo(const DriveInfo& drive);

	void ReadTrackInfo(const QSharedPointer<DatabaseFacade>& adapter,
		QString const& file_path,
		int32_t playlist_id,
		bool is_podcast_mode);

	void SearchLyrics(int32_t music_id, const QString& title, const QString& artist);

public slots:
    void play(const PlayListEntity& item);

	void PlayPlayListEntity(const PlayListEntity& item);

    void AddPlaylistItem(const ForwardList<int32_t>& music_ids, const ForwardList<PlayListEntity>& entities);

	void OnArtistIdChanged(const QString& artist, const QString& cover_id, int32_t artist_id);

	void ProcessTrackInfo(int32_t total_album, int32_t total_tracks) const;

	void OnActivated(QSystemTrayIcon::ActivationReason reason);

	void OnVolumeChanged(float volume);

	void SetCover(const QString& cover_id, PlaylistPage* page);

	void OnClickedAlbum(const QString& album, int32_t album_id, const QString& cover_id);

	void OnUpdateCdTrackInfo(const QString& disc_id, const ForwardList<TrackInfo>& track_infos);

	void OnUpdateMbDiscInfo(const MbDiscIdInfo& mb_disc_id_info);

	void OnUpdateDiscCover(const QString& disc_id, const QString& cover_id);

	void OnSearchLyricsCompleted(int32_t music_id, const QString& lyrics, const QString& trlyrics);

	void OnCurrentThemeChanged(ThemeColor theme_color);

private:
	void DrivesChanges(const QList<DriveInfo>& drive_infos) override;

	void DrivesRemoved(const DriveInfo& drive_info) override;

	bool HitTitleBar(const QPoint& ps) const override;

    void stop() override;

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

	void InitialDeviceList();

	void InitialShortcut();

	void InitialSpectrum();

	void PlayNextItem(int32_t forward);

    void SetTablePlaylistView(int table_id, ConstLatin1String column_setting_name);

	void SetPlayerOrder();

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

	void ExtractFile(const QString &file_path);

	PlaylistPage* CurrentPlaylistPage();

	void cleanup();

    void SetupDsp(const PlayListEntity& item);

	void ConnectPlaylistPageSignal(PlaylistPage* playlist_page);

	void AppendToPlaylist(const QString& file_name);

	void SliderAnimation(bool enable);

	QString TranslateErrorCode(const Errors error) const;

	void SetupSampleWriter(Pcm2DsdConvertModes convert_mode,
		DsdModes dsd_modes,
		int32_t input_sample_rate,
		ByteFormat byte_format,
		PlaybackFormat& playback_format) const;

	void SetupSampleRateConverter(std::function<void()>& initial_sample_rate_converter,
		uint32_t& target_sample_rate,
		QString& sample_rate_converter_type);

	static bool ShowMeMessage(const QString &message);

	void showEvent(QShowEvent* event) override;

	bool is_seeking_;
	PlayerOrder order_;
	LrcPage* lrc_page_;
	PlaylistPage* playlist_page_;
	PlaylistPage* podcast_page_;
	PlaylistPage* music_page_;
	CdPage* cd_page_;
	PlaylistPage* current_playlist_page_;
	AlbumArtistPage* album_page_;
	FileSystemViewPage* file_system_view_page_;
	IXMainWindow* main_window_;
	BackgroundWorker* background_worker_;
	XMessage* messages_;
	QModelIndex play_index_;
	DeviceInfo device_info_;
	PlayListEntity current_entity_;
    QStack<int32_t> stack_page_id_;
    QThread background_thread_;
	std::shared_ptr<UIPlayerStateAdapter> state_adapter_;
	std::shared_ptr<IAudioPlayer> player_;
	QVector<QFrame*> device_type_frame_;
    Ui::XampWindow ui_;
};
