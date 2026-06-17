//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QString>
#include <QList>
#include <QPixmap>

#include <expected>
#include <optional>
#include <qmetatype.h>

#include <widget/widget_shared_global.h>

namespace musicbrain {
    enum class ParserError {
        PARSE_ERROR_JSON_ERROR
    };

    struct XAMP_WIDGET_SHARED_API TagInfo {
        QString name;
        int count = 0;
        QString id;
        QString disambiguation;
    };

    struct XAMP_WIDGET_SHARED_API Artist {
        QString id;
        QString name;
        QString sortName;
        QString disambiguation;
        QString country;
        QString type;
        QString typeId;
        QList<TagInfo> tags;
        QList<TagInfo> genres;
    };

    struct XAMP_WIDGET_SHARED_API ArtistCredit {
        QString joinPhrase;
        QString name;
        Artist artist;
    };

    struct XAMP_WIDGET_SHARED_API TextRepresentation {
        QString language; // e.g. "jpn"
        QString script;   // e.g. "Jpan"
    };

    struct XAMP_WIDGET_SHARED_API Area {
        QString id;            // "2db42837-c832-3c27-b4a3-08198f75693c"
        QString name;          // "Japan" / "[Worldwide]"
        QString sortName;      // "Japan" / "[Worldwide]"
        QStringList iso3166_1; // ["JP"] / ["XW"]
        QString type;
        QString typeId;
        QString disambiguation;
    };

    struct XAMP_WIDGET_SHARED_API ReleaseEvent {
        Area area;
        QString date; // "YYYY-MM-DD" 可能只到 YYYY 或 YYYY-MM
    };

    struct XAMP_WIDGET_SHARED_API ReleaseGroup {
        QString id;                 // "release-group" -> id
        QString title;              // "release-group" -> title
        QString primaryType;        // "primary-type"
        QString primaryTypeId;      // "primary-type-id"
        QStringList secondaryTypes; // "secondary-types" (array of strings)
        QStringList secondaryTypeIds; // "secondary-type-ids" (array of strings)
        QString firstReleaseDate;   // "first-release-date"
        QString disambiguation;     // disambiguation
        int trackCount = 0; // total tracks of this release
        QString date; // ISO-ish date
        QString country; // e.g., "JP"
        QStringList mediaFormats; // per medium, aggregated names, e.g., {"CD","CD"}
        bool loadedInGroup = false; // true if already loaded in your app
    };

    struct XAMP_WIDGET_SHARED_API Release {
        QString id;
        QString title;
        QString status;      // "Official"
        QString statusId;
        QString country;     // "JP" / "XW"
        QString barcode;
        QString packaging;   // "None"/null
        QString packagingId;
        QString disambiguation; // 例如 "期間生産限定盤"
        QString date;           // 針對 release.level 的 date（常與第一個 event 同步）
        TextRepresentation textRep;
        QList<ReleaseEvent> events;
        QString quality; // "normal"
        ReleaseGroup releaseGroup;
        QList<ArtistCredit> artistCredits;
        double mbSearchScore = 0; // mb search api 回傳的 score
        int lengthMs = 0;
        int trackCount = 0; // total tracks of this release
        QStringList mediaFormats; // per medium, aggregated names, e.g., {"CD","CD"}
        bool loadedInGroup = false; // true if already loaded in your app
    };

    struct XAMP_WIDGET_SHARED_API RootRecording {
        QString id;
        QString title;
        bool video = false;
        QString disambiguation;
        int lengthMs = 0;
        QList<ArtistCredit> artistCredits;
        QList<TagInfo> tags;
        QList<TagInfo> genres;
		double mbSearchScore = 0; // mb search api 回傳的 score
        QString firstReleaseDate; // "first-release-date"
        QList<Release> releases;  // inc=releases 時才會有
    };

    struct XAMP_WIDGET_SHARED_API TrackInfo {
        QString id;
        int disc{};
        int trackNo{};
        bool video{ false };
        QString title;
        int lengthMs{};
        QString recordingId;
        QStringList artistCredits;
        QList<Release> releases;
    };

    struct XAMP_WIDGET_SHARED_API FileMeta {
        QString title;
        QString album;
        QString artist;
        QString albumArtist;
        QString date;
        QString barcode;
        QString recordingId;
        QString trackId;
        int trackNumber = 0;
        int totalTracks = 0;
        int discNumber = 0;
        int totalDiscs = 0;
        int totalAlbumTracks = 0;
        int lengthMs = 0;
        bool video = false;
    };

    struct XAMP_WIDGET_SHARED_API TrackMatchResult {
        double similarity = 0;
        int fileIndex = -1;
        int trackIndex = -1;
    };

    std::expected<RootRecording, ParserError> parseRootRecording(const QString& jsonText);

    std::expected<QList<RootRecording>, ParserError> parseRootRecordingList(const QString& jsonText);

    std::expected<QList<Release>, ParserError> parseReleaseList(const QString& jsonText);

    std::optional<QList<TrackInfo>> parseReleaseTracklist(const QByteArray& json, const QList<Release> &releases);

    XAMP_WIDGET_SHARED_API double lengthScore(int a, int b);

    XAMP_WIDGET_SHARED_API double compareToRelease(const FileMeta& meta, const Release& release);

    XAMP_WIDGET_SHARED_API double compareToRecording(const FileMeta& meta, const RootRecording& recording);

    XAMP_WIDGET_SHARED_API double compareToTrack(const FileMeta& meta, const TrackInfo& track);
}

struct XAMP_WIDGET_SHARED_API MusicBrainzRecording {
    QString release_id;
    QString title;
    QPixmap cover_art;
    double similarity = 0;
    musicbrain::RootRecording root_recording;
    QList<musicbrain::TrackInfo> tracks;
};

struct XAMP_WIDGET_SHARED_API MusicBrainzAlbum {
    QList<MusicBrainzRecording> recordings;

    MusicBrainzAlbum() = default;
};

Q_DECLARE_METATYPE(musicbrain::TrackInfo)
Q_DECLARE_METATYPE(MusicBrainzRecording)
Q_DECLARE_METATYPE(MusicBrainzAlbum)
