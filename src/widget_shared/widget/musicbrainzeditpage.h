//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <thememanager.h>
#include <QCoroTask>
#include <QHash>
#include <QNetworkAccessManager>
#include <QSet>
#include <widget/widget_shared_global.h>
#include <widget/httpx.h>
#include <widget/musicbrainzparser.h>
#include <widget/playlistentity.h>

#include <optional>

namespace Ui {
    class MusicbrainzEditPage;
}

class QPushButton;
class QProgressBar;
class QStandardItemModel;
class QCloseEvent;
class QLabel;

class XAMP_WIDGET_SHARED_EXPORT MusicbrainzEditPage final : public QFrame {
    Q_OBJECT
public:
    MusicbrainzEditPage(const QList<PlayListEntity>& entities, QWidget* parent = nullptr);

    virtual ~MusicbrainzEditPage() override;

public slots:
    void onThemeChangedFinished(ThemeColor theme_color);

    void onRetranslateUi();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void load(const QList<PlayListEntity>& entities);
    void rebuildCandidateView();
    void appendMusicBrainzAlbum(const MusicBrainzAlbum& album);
    std::optional<PlayListEntity> entityForTrack(int track_no) const;
    void selectTrackViewTrack(int track_no);
    void selectTrackViewEntity(int32_t music_id);
    void updateFetchProgressText(const QString& state, bool use_release_progress = false);
    void updateWriteProgressText(const QString& state);
    void setTagPreview(const std::optional<PlayListEntity>& entity,
        const std::optional<musicbrain::TrackInfo>& track,
        const QString& album);
    void setCoverPreview(QLabel* image_label, QLabel* info_label, const QPixmap& image, size_t image_file_size);
    void updateOriginalCoverArt(const std::optional<PlayListEntity>& entity);
    void updateNewCoverArt(const QString& release_id);
    QPixmap coverArtForRelease(const QString& release_id) const;
    size_t coverArtFileSizeForRelease(const QString& release_id) const;
    std::optional<PlayListEntity> entityForExactTrack(int track_no) const;
    void updateWriteTagButtons();
    void writeSelectedTag();
    void writeSelectedAlbumTags();
    QCoro::Task<> startFetchMusicBrainzRecording();
    QCoro::Task<QList<musicbrain::Release>> fetchCandidateReleases(const QList<PlayListEntity>& entities);
    QCoro::Task<bool> fetchMusicBrainzRelease(const QList<PlayListEntity>& entities,
        const QList<musicbrain::Release>& candidate_releases,
        QSet<QString>& fetched_release_ids,
        QHash<QString, QList<musicbrain::TrackInfo>>& release_track_cache,
        QHash<QString, QPixmap>& release_cover_cache,
        QHash<QString, size_t>& release_cover_size_cache);
    QCoro::Task<std::optional<QByteArray>> fetchCoverArtByUrl(const QString& tag, const QString& release_id, size_t prefer_size = 1200);
    QCoro::Task<std::optional<QByteArray>> tryFetchCoverArt(const QString& tag, const QString& release_id, size_t size);

    QStandardItemModel* track_model_;
    QStandardItemModel* album_track_model_;
    QStandardItemModel* tag_model_;
    QList<PlayListEntity> entities_;
    QList<musicbrain::FileMeta> metas_;
    QList<MusicBrainzAlbum> recording_list_;
    QMap<QString, QPixmap> cover_art_map_;
    QMap<QString, size_t> cover_art_size_map_;
    QNetworkAccessManager nam_;
    http::HttpClient http_client_;
    Ui::MusicbrainzEditPage* ui_;
    QProgressBar* fetch_progress_bar_{ nullptr };
    QPushButton* write_tag_button_{ nullptr };
    QPushButton* write_album_tags_button_{ nullptr };
    std::optional<PlayListEntity> selected_entity_;
    std::optional<musicbrain::TrackInfo> selected_track_;
    QString selected_album_;
    QString selected_release_id_;
    int completed_albums_{ 0 };
    int total_albums_{ 0 };
    int completed_recordings_{ 0 };
    int total_recordings_{ 0 };
    int completed_releases_{ 0 };
    int total_releases_{ 0 };
    int completed_writes_{ 0 };
    int total_writes_{ 0 };
    bool is_fetching_{ false };
    bool is_writing_{ false };
};
