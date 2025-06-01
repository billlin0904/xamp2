//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QString>
#include <QList>

#include <expected>
#include <optional>

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

    struct TagInfo {
        QString name;
        int count = 0;
        QString id;
        QString disambiguation;
    };

    struct Artist {
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

    struct ArtistCredit {
        QString joinPhrase;
        QString name;
        Artist artist;
    };

    struct RootRecording {
        QString id;
        QString title;
        bool video = false;
        QString disambiguation;
        int length = 0;
        QList<ArtistCredit> artistCredits;
        QList<TagInfo> tags;
        QList<TagInfo> genres;
    };

    std::expected<RootRecording, ParserError> parseRootRecording(const QString& jsonText);
}
