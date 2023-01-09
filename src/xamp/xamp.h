//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QStack>
#include <QThread>
#include <QSystemTrayIcon>

#include <widget/widget_shared.h>
#include <base/encodingprofile.h>
#include <widget/uiplayerstateadapter.h>
#include <widget/playlistentity.h>
#include <widget/playerorder.h>
#include <widget/albumentity.h>
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
class ToolTipsFilter;
class QRadioButton;
class BackgroundWorker;
class CdPage;
class XMenu;
class MetadataExtractAdapter;

class Xamp final : public IXPlayerControlFrame {
	Q_OBJECT

public:
    explicit Xamp(const std::shared_ptr<IAudioPlayer> &player);

    virtual ~Xamp() override;

    void setXWindow(IXWindow* top_window);

    void applyTheme(QColor backgroundColor, QColor color);

	void shortcutsPressed(const QKeySequence& shortcut) override;
signals:
	void payNextMusic();

    void themeChanged(QColor backgroundColor, QColor color);

	void nowPlaying(QString const& artist, QString const& title);

	void addBlurImage(const QString& cover_id, const QPixmap& image, QSize size);

	void fetchCdInfo(const DriveInfo& drive);

	void readFileMetadata(const QSharedPointer<MetadataExtractAdapter>& adapter,
		QString const& file_path,
		int32_t playlist_id,
		bool is_podcast_mode);
public slots:
    void playAlbumEntity(const AlbumEntity& item);

	void playPlayListEntity(const PlayListEntity& item);

    void addPlaylistItem(const ForwardList<int32_t>& music_ids, const ForwardList<PlayListEntity>& entities);

	void onArtistIdChanged(const QString& artist, const QString& cover_id, int32_t artist_id);

	void processMeatadata(int64_t dir_last_write_time, const ForwardList<TrackInfo>& medata) const;

	void onActivated(QSystemTrayIcon::ActivationReason reason);

	void onVolumeChanged(float volume);

	void setCover(const QString& cover_id, PlaylistPage* page);

	void onClickedAlbum(const QString& album, int32_t album_id, const QString& cover_id);

	void onUpdateCdMetadata(const QString& disc_id, const ForwardList<TrackInfo>& metadatas);

	void onUpdateMbDiscInfo(const MbDiscIdInfo& mb_disc_id_info);

	void onUpdateDiscCover(const QString& disc_id, const QString& cover_id);

private:
	void drivesChanges(const QList<DriveInfo>& drive_infos) override;

	void drivesRemoved(const DriveInfo& drive_info) override;

	bool hitTitleBar(const QPoint& ps) const override;

    void stop() override;

    void playNext() override;

    void playPrevious() override;

    void playOrPause() override;

    void deleteKeyPress() override;

    void addDropFileItem(const QUrl& url) override;

	void closeEvent(QCloseEvent* event) override;

	void updateMaximumState(bool is_maximum) override;

	void systemThemeChanged(ThemeColor theme_color) override;

    void focusIn() override;

    void focusOut() override;

	void setPlaylistPageCover(const QPixmap* cover, PlaylistPage* page = nullptr);

	QWidgetAction* createTextSeparator(const QString& desc);

	void onSampleTimeChanged(double stream_time);

	void playLocalFile(const PlayListEntity& item);

	void onPlayerStateChanged(PlayerState play_state);

	void addItem(const QString& file_name);

    void setVolume(uint32_t volume);

	void setCurrentTab(int32_t table_id);

	void initialUI();

	void initialPlaylist();

	void initialController();

	void initialDeviceList();

	void initialShortcut();

	void initialSpectrum();

	void playNextItem(int32_t forward);

    void setTablePlaylistView(int table_id, ConstLatin1String column_setting_name);

	void setPlayerOrder();

	PlaylistPage* newPlaylistPage(int32_t playlist_id, const QString& column_setting_name);

	void pushWidget(QWidget* widget);

	QWidget* popWidget();

	QWidget* topWidget();

	void goBackPage();

	void getNextPage();

	void setSeekPosValue(double stream_time_as_ms);

	void resetSeekPosValue();

    void onDeviceStateChanged(DeviceState state);

    void encodeFlacFile(const PlayListEntity& item);

	void encodeAACFile(const PlayListEntity& item, const EncodingProfile & profile);

	void encodeWavFile(const PlayListEntity& item);

    void updateUI(const AlbumEntity& item, const PlaybackFormat& playback_format, bool open_done);

	void updateButtonState();

	void extractFile(const QString &file_path);

	PlaylistPage* currentPlyalistPage();

	void cleanup();

    void setupDSP(const AlbumEntity& item);

	void avoidRedrawOnResize();

	void connectPlaylistPageSignal(PlaylistPage* playlist_page);

	void setTipHint(QWidget* widget, const QString& hint_text);

	void appendToPlaylist(const QString& file_name);

	void sliderAnimation(bool enable);

	QString translateErrorCode(const Errors error);

	void setupSampleWriter(PlaybackFormat& playback_format, QString& samplerate_converter_type, ByteFormat byte_format);

	void setupSampleRateConverter(std::function<void()>& initial_sample_rate_converter,
		uint32_t& target_sample_rate,
		QString& sample_rate_converter_type);

	bool showMeMessage(const QString &message);

	bool is_seeking_;
	PlayerOrder order_;
	LrcPage* lrc_page_;
	PlaylistPage* playlist_page_;
	PlaylistPage* podcast_page_;
	PlaylistPage* music_page_;
	CdPage* cd_page_;
	PlaylistPage* current_playlist_page_;
	AlbumArtistPage* album_page_;
    ArtistInfoPage* artist_info_page_;
	PreferencePage* preference_page_;
	FileSystemViewPage* file_system_view_page_;
	AboutPage* about_page_;
	IXWindow* top_window_;
	ToolTipsFilter* tool_tips_filter_;
	BackgroundWorker* background_worker_;
	QModelIndex play_index_;
	DeviceInfo device_info_;
	PlayListEntity current_entity_;
    QStack<int32_t> stack_page_id_;
    QThread background_thread_;
	std::shared_ptr<UIPlayerStateAdapter> state_adapter_;
	std::shared_ptr<IAudioPlayer> player_;
    Ui::XampWindow ui_;
};
