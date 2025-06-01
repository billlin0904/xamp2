#include <widget/musicbrainzparser.h>
#include <widget/util/str_util.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QDebug>

namespace acoustid {

    // 解析子項：Artist
    static Artist parseArtist(const QJsonObject& obj) {
        Artist artist;
        artist.id = obj.value("id"_str).toString();
        artist.name = obj.value("name"_str).toString();
        // 有些項目裡有 "joinphrase"
        artist.joinPhrase = obj.value("joinphrase"_str).toString();
        return artist;
    }

    // 解析子項：ReleaseDate
    static ReleaseDate parseDate(const QJsonObject& obj) {
        ReleaseDate date;
        date.day = obj.value("day"_str).toInt(0);
        date.month = obj.value("month"_str).toInt(0);
        date.year = obj.value("year"_str).toInt(0);
        return date;
    }

    // 解析子項：Track
    static Track parseTrack(const QJsonObject& obj) {
        Track track;
        track.id = obj.value("id"_str).toString();
        track.position = obj.value("position"_str).toInt(0);

        if (obj.contains("artists"_str) && obj["artists"_str].isArray()) {
            QJsonArray arr = obj["artists"_str].toArray();
            for (auto v : arr) {
                if (v.isObject()) {
                    track.artists.append(parseArtist(v.toObject()));
                }
            }
        }
        return track;
    }

    // 解析子項：Medium
    static Medium parseMedium(const QJsonObject& obj) {
        Medium medium;
        medium.format = obj.value("format"_str).toString();
        medium.position = obj.value("position"_str).toInt(0);
        medium.trackCount = obj.value("track_count"_str).toInt(0);

        // tracks
        if (obj.contains("tracks"_str) && obj["tracks"_str].isArray()) {
            QJsonArray tracksArray = obj["tracks"_str].toArray();
            for (auto v : tracksArray) {
                if (v.isObject()) {
                    medium.tracks.append(parseTrack(v.toObject()));
                }
            }
        }
        return medium;
    }

    // 解析子項：ReleaseEvent
    static ReleaseEvent parseReleaseEvent(const QJsonObject& obj) {
        ReleaseEvent e;
        e.country = obj.value("country"_str).toString();
        if (obj.contains("date"_str) && obj["date"_str].isObject()) {
            e.date = parseDate(obj["date"_str].toObject());
        }
        return e;
    }

    // 解析子項：Release
    static Release parseRelease(const QJsonObject& obj) {
        Release release;
        release.id = obj.value("id"_str).toString();
        release.country = obj.value("country"_str).toString();

        // date
        if (obj.contains("date"_str) && obj["date"_str].isObject()) {
            release.date = parseDate(obj["date"_str].toObject());
        }

        release.mediumCount = obj.value("medium_count"_str).toInt(0);
        release.trackCount = obj.value("track_count"_str).toInt(0);

        // mediums
        if (obj.contains("mediums"_str) && obj["mediums"_str].isArray()) {
            QJsonArray mediumsArray = obj["mediums"_str].toArray();
            for (auto v : mediumsArray) {
                if (v.isObject()) {
                    release.mediums.append(parseMedium(v.toObject()));
                }
            }
        }

        // releaseevents
        if (obj.contains("releaseevents"_str) && obj["releaseevents"_str].isArray()) {
            QJsonArray eventsArray = obj["releaseevents"_str].toArray();
            for (auto v : eventsArray) {
                if (v.isObject()) {
                    release.releaseEvents.append(parseReleaseEvent(v.toObject()));
                }
            }
        }

        return release;
    }

    // 解析子項：ReleaseGroup
    static ReleaseGroup parseReleaseGroup(const QJsonObject& obj) {
        ReleaseGroup rg;
        rg.id = obj.value("id"_str).toString();
        rg.title = obj.value("title"_str).toString();
        rg.type = obj.value("type"_str).toString();

        // releases
        if (obj.contains("releases"_str) && obj["releases"_str].isArray()) {
            QJsonArray arr = obj["releases"_str].toArray();
            for (auto v : arr) {
                if (v.isObject()) {
                    rg.releases.append(parseRelease(v.toObject()));
                }
            }
        }
        return rg;
    }

    // 解析子項：Recording
    static Recording parseRecording(const QJsonObject& obj) {
        Recording rec;
        rec.id = obj.value("id"_str).toString();
        rec.duration = obj.value("duration"_str).toInt(0);
        rec.title = obj.value("title"_str).toString();

        // artists
        if (obj.contains("artists"_str) && obj["artists"_str].isArray()) {
            QJsonArray artistsArray = obj["artists"_str].toArray();
            for (auto v : artistsArray) {
                if (v.isObject()) {
                    rec.artists.append(parseArtist(v.toObject()));
                }
            }
        }

        // releasegroups
        if (obj.contains("releasegroups"_str) && obj["releasegroups"_str].isArray()) {
            QJsonArray groupsArray = obj["releasegroups"_str].toArray();
            for (auto v : groupsArray) {
                if (v.isObject()) {
                    rec.releaseGroups.append(parseReleaseGroup(v.toObject()));
                }
            }
        }

        return rec;
    }

    // 解析子項：Result
    static Result parseResult(const QJsonObject& obj) {
        Result r;
        r.id = obj.value("id"_str).toString();
        r.score = obj.value("score"_str).toDouble(0.0);

        // recordings
        if (obj.contains("recordings"_str) && obj["recordings"_str].isArray()) {
            QJsonArray recArray = obj["recordings"_str].toArray();
            for (auto v : recArray) {
                if (v.isObject()) {
                    r.recordings.append(parseRecording(v.toObject()));
                }
            }
        }

        return r;
    }

    std::expected<AcoustidResponse, ParserError> parseAcoustidResponse(const QString& jsonText) {
        AcoustidResponse resp;

        QJsonDocument doc = QJsonDocument::fromJson(jsonText.toUtf8());
        if (doc.isNull() || !doc.isObject()) {
            return std::unexpected(ParserError::PARSE_ERROR_JSON_ERROR);
        }

        QJsonObject rootObj = doc.object();

        resp.status = rootObj.value("status"_str).toString();

        QJsonValue resultsVal = rootObj.value("results"_str);
        if (resultsVal.isArray()) {
            QJsonArray resultsArray = resultsVal.toArray();
            for (auto v : resultsArray) {
                if (v.isObject()) {
                    resp.results.append(parseResult(v.toObject()));
                }
            }
        }
        return resp;
    }
}

namespace musicbrain {
    static TagInfo parseTagInfo(const QJsonObject& obj) {
        TagInfo tag;
        tag.name = obj.value("name"_str).toString();
        tag.count = obj.value("count"_str).toInt(0);
        tag.id = obj.value("id"_str).toString();
        tag.disambiguation = obj.value("disambiguation"_str).toString();
        return tag;
    }

    static Artist parseArtist(const QJsonObject& obj) {
        Artist a;
        a.id = obj.value("id"_str).toString();
        a.name = obj.value("name"_str).toString();
        a.sortName = obj.value("sort-name"_str).toString();
        a.disambiguation = obj.value("disambiguation"_str).toString();
        a.country = obj.value("country"_str).toString();
        a.type = obj.value("type"_str).toString();
        a.typeId = obj.value("type-id"_str).toString();

        // 解析 "tags"
        if (obj.contains("tags"_str) && obj["tags"_str].isArray()) {
            QJsonArray tagsArray = obj["tags"_str].toArray();
            for (auto v : tagsArray) {
                if (v.isObject()) {
                    a.tags.append(parseTagInfo(v.toObject()));
                }
            }
        }

        // 解析 "genres"
        if (obj.contains("genres"_str) && obj["genres"_str].isArray()) {
            QJsonArray genresArray = obj["genres"_str].toArray();
            for (auto v : genresArray) {
                if (v.isObject()) {
                    a.genres.append(parseTagInfo(v.toObject()));
                }
            }
        }

        return a;
    }

    static ArtistCredit parseArtistCredit(const QJsonObject& obj) {
        ArtistCredit ac;
        ac.joinPhrase = obj.value("joinphrase"_str).toString();
        ac.name = obj.value("name"_str).toString();

        // 解析 artist 子物件
        if (obj.contains("artist"_str) && obj["artist"_str].isObject()) {
            ac.artist = parseArtist(obj["artist"_str].toObject());
        }

        return ac;
    }

    std::expected<RootRecording, ParserError> parseRootRecording(const QString& jsonText) {
        RootRecording rec;

        // 將 JSON 字串轉成 QJsonDocument
        QJsonDocument doc = QJsonDocument::fromJson(jsonText.toUtf8());
        if (doc.isNull() || !doc.isObject()) {            
            return std::unexpected(ParserError::PARSE_ERROR_JSON_ERROR);
        }

        QJsonObject rootObj = doc.object();

        // 1) 基本欄位
        rec.id = rootObj.value("id"_str).toString();
        rec.title = rootObj.value("title"_str).toString();
        rec.video = rootObj.value("video"_str).toBool(false);
        rec.disambiguation = rootObj.value("disambiguation"_str).toString();
        rec.length = rootObj.value("length"_str).toInt(0);

        // 2) "artist-credit" -> array
        if (rootObj.contains("artist-credit"_str) && rootObj["artist-credit"_str].isArray()) {
            QJsonArray acArray = rootObj["artist-credit"_str].toArray();
            for (auto v : acArray) {
                if (v.isObject()) {
                    rec.artistCredits.append(parseArtistCredit(v.toObject()));
                }
            }
        }

        // 3) "tags" -> array
        if (rootObj.contains("tags"_str) && rootObj["tags"_str].isArray()) {
            QJsonArray tagsArr = rootObj["tags"_str].toArray();
            for (auto v : tagsArr) {
                if (v.isObject()) {
                    rec.tags.append(parseTagInfo(v.toObject()));
                }
            }
        }

        // 4) "genres" -> array
        if (rootObj.contains("genres"_str) && rootObj["genres"_str].isArray()) {
            QJsonArray genresArr = rootObj["genres"_str].toArray();
            for (auto v : genresArr) {
                if (v.isObject()) {
                    rec.genres.append(parseTagInfo(v.toObject()));
                }
            }
        }

        return rec;
    }
}