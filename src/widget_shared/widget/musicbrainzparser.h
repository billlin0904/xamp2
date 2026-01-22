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

namespace acoustid {
    enum class ParserError {
        PARSE_ERROR_JSON_ERROR
    };

    struct Artist {
        QString id;
        QString name;
        QString joinPhrase;
    };

    struct Track {
        QString id;
        int position = 0;
        QList<Artist> artists;
    };

    struct Medium {
        QString format;
        int position = 0;
        int trackCount = 0;
        QList<Track> tracks;
    };

    struct ReleaseDate {
        int day = 0;
        int month = 0;
        int year = 0;
    };

    struct ReleaseEvent {
        QString country;
        ReleaseDate date;
    };

    struct Release {
        QString id;
        QString country;
        ReleaseDate date;
        int mediumCount = 0;
        QList<Medium> mediums;
        QList<ReleaseEvent> releaseEvents;
        int trackCount = 0;
    };

    struct ReleaseGroup {
        QString id;
        QString title;
        QString type;
        QList<Release> releases;
    };

    struct Recording {
        QString id;
        QList<Artist> artists;
        int duration = 0;
        QList<ReleaseGroup> releaseGroups;
        QString title;
    };

    struct Result {
        QString id;
        QList<Recording> recordings;
        double score = 0.0;
    };

    struct AcoustidResponse {
        QString status;
        QList<Result> results;
    };

    std::expected<AcoustidResponse, ParserError> parseAcoustidResponse(const QString& jsonText);
}

namespace musicbrain {
    enum class ParserError {
        PARSE_ERROR_JSON_ERROR
    };

    struct XAMP_WIDGET_SHARED_EXPORT TagInfo {
        QString name;
        int count = 0;
        QString id;
        QString disambiguation;
    };

    struct XAMP_WIDGET_SHARED_EXPORT Artist {
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

    struct XAMP_WIDGET_SHARED_EXPORT ArtistCredit {
        QString joinPhrase;
        QString name;
        Artist artist;
    };

    struct XAMP_WIDGET_SHARED_EXPORT TextRepresentation {
        QString language; // e.g. "jpn"
        QString script;   // e.g. "Jpan"
    };

    struct XAMP_WIDGET_SHARED_EXPORT Area {
        QString id;            // "2db42837-c832-3c27-b4a3-08198f75693c"
        QString name;          // "Japan" / "[Worldwide]"
        QString sortName;      // "Japan" / "[Worldwide]"
        QStringList iso3166_1; // ["JP"] / ["XW"]
        QString type;
        QString typeId;
        QString disambiguation;
    };

    struct XAMP_WIDGET_SHARED_EXPORT ReleaseEvent {
        Area area;
        QString date; // "YYYY-MM-DD" 可能只到 YYYY 或 YYYY-MM
    };

    struct XAMP_WIDGET_SHARED_EXPORT ReleaseGroup {
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

    struct XAMP_WIDGET_SHARED_EXPORT Release {
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
        double mbSearchScore; // mb search api 回傳的 score
        int lengthMs = 0;
        int trackCount = 0; // total tracks of this release
        QStringList mediaFormats; // per medium, aggregated names, e.g., {"CD","CD"}
        bool loadedInGroup = false; // true if already loaded in your app
    };

    struct XAMP_WIDGET_SHARED_EXPORT RootRecording {
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

    struct XAMP_WIDGET_SHARED_EXPORT TrackInfo {
        int disc{};
        int trackNo{};
        bool video{ false };
        QString title;
        int lengthMs{};
        QString recordingId;
        QStringList artistCredits;
        QList<Release> releases;
    };

    std::expected<RootRecording, ParserError> parseRootRecording(const QString& jsonText);

    std::optional<QList<TrackInfo>> parseReleaseTracklist(const QByteArray& json, const QList<Release> &releases);
}

struct XAMP_WIDGET_SHARED_EXPORT MusicBrainzRecording {
    QString release_id;
    QString title;
    QPixmap cover_art;
    musicbrain::RootRecording root_recording;
    QList<musicbrain::TrackInfo> tracks;
};

struct XAMP_WIDGET_SHARED_EXPORT MusicBrainzAlbum {
    QList<MusicBrainzRecording> recordings;

    MusicBrainzAlbum() = default;
};

Q_DECLARE_METATYPE(musicbrain::TrackInfo)
Q_DECLARE_METATYPE(MusicBrainzRecording)
Q_DECLARE_METATYPE(MusicBrainzAlbum)