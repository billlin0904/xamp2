#include <widget/musicbrainzparser.h>
#include <widget/util/str_util.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QDebug>

namespace acoustid {

    // 解析子項：Artist
    Artist parseArtist(const QJsonObject& obj) {
        Artist artist;
        artist.id = obj.value("id"_str).toString();
        artist.name = obj.value("name"_str).toString();
        // 有些項目裡有 "joinphrase"
        artist.joinPhrase = obj.value("joinphrase"_str).toString();
        return artist;
    }

    // 解析子項：ReleaseDate
    ReleaseDate parseDate(const QJsonObject& obj) {
        ReleaseDate date;
        date.day = obj.value("day"_str).toInt(0);
        date.month = obj.value("month"_str).toInt(0);
        date.year = obj.value("year"_str).toInt(0);
        return date;
    }

    // 解析子項：Track
    Track parseTrack(const QJsonObject& obj) {
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
    Medium parseMedium(const QJsonObject& obj) {
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
    ReleaseEvent parseReleaseEvent(const QJsonObject& obj) {
        ReleaseEvent e;
        e.country = obj.value("country"_str).toString();
        if (obj.contains("date"_str) && obj["date"_str].isObject()) {
            e.date = parseDate(obj["date"_str].toObject());
        }
        return e;
    }

    // 解析子項：Release
    Release parseRelease(const QJsonObject& obj) {
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
    

    // 解析子項：Recording
    Recording parseRecording(const QJsonObject& obj) {
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

        return rec;
    }

    // 解析子項：Result
    Result parseResult(const QJsonObject& obj) {
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
    TextRepresentation parseTextRep(const QJsonObject& obj) {
        TextRepresentation tr;
        tr.language = obj.value("language"_str).toString();
        tr.script = obj.value("script"_str).toString();
        return tr;
    }

    Area parseArea(const QJsonObject& obj) {
        Area a;
        a.id = obj.value("id"_str).toString();
        a.name = obj.value("name"_str).toString();
        a.sortName = obj.value("sort-name"_str).toString();
        a.type = obj.value("type"_str).toString();
        a.typeId = obj.value("type-id"_str).toString();
        a.disambiguation = obj.value("disambiguation"_str).toString();
        // iso-3166-1-codes -> array of strings
        if (obj.contains("iso-3166-1-codes"_str) && obj["iso-3166-1-codes"_str].isArray()) {
            for (const auto& v : obj["iso-3166-1-codes"_str].toArray()) {
                a.iso3166_1.append(v.toString());
            }
        }
        return a;
    }

    ReleaseEvent parseReleaseEvent(const QJsonObject& obj) {
        ReleaseEvent e;
        e.date = obj.value("date"_str).toString();
        if (obj.contains("area"_str) && obj["area"_str].isObject()) {
            e.area = parseArea(obj["area"_str].toObject());
        }
        return e;
    }

    ReleaseGroup parseReleaseGroup(const QJsonObject& obj) {
        ReleaseGroup g;
        g.id = obj.value("id"_str).toString();
        g.title = obj.value("title"_str).toString();
        g.primaryType = obj.value("primary-type"_str).toString();
        g.primaryTypeId = obj.value("primary-type-id"_str).toString();
        g.firstReleaseDate = obj.value("first-release-date"_str).toString();
        g.disambiguation = obj.value("disambiguation"_str).toString();

        // "secondary-types": [ "Live", "Compilation", ... ]
        if (obj.contains("secondary-types"_str) && obj["secondary-types"_str].isArray()) {
            const auto arr = obj["secondary-types"_str].toArray();
            for (const auto& v : arr) g.secondaryTypes.append(v.toString());
        }

        // "secondary-type-ids": [ "...", "..." ]
        if (obj.contains("secondary-type-ids"_str) && obj["secondary-type-ids"_str].isArray()) {
            const auto arr = obj["secondary-type-ids"_str].toArray();
            for (const auto& v : arr) g.secondaryTypeIds.append(v.toString());
        }

        return g;
    }

    Release parseRelease(const QJsonObject& obj) {
        Release r;
        r.id = obj.value("id"_str).toString();
        r.title = obj.value("title"_str).toString();
        r.status = obj.value("status"_str).toString();
        r.statusId = obj.value("status-id"_str).toString();
        r.country = obj.value("country"_str).toString();
        r.barcode = obj.value("barcode"_str).toString();
        r.packaging = obj.value("packaging"_str).toString();
        r.packagingId = obj.value("packaging-id"_str).toString();
        r.disambiguation = obj.value("disambiguation"_str).toString();
        r.quality = obj.value("quality"_str).toString();
        r.date = obj.value("date"_str).toString();

        if (obj.contains("text-representation"_str) && obj["text-representation"_str].isObject()) {
            r.textRep = parseTextRep(obj["text-representation"_str].toObject());
        }
        if (obj.contains("release-events"_str) && obj["release-events"_str].isArray()) {
            for (const auto& v : obj["release-events"_str].toArray()) {
                if (v.isObject()) r.events.append(parseReleaseEvent(v.toObject()));
            }
        }
        if (obj.contains("release-group"_str) && obj["release-group"_str].isObject()) {
            r.releaseGroup = parseReleaseGroup(obj["release-group"_str].toObject());
        }
        return r;
    }

    TagInfo parseTagInfo(const QJsonObject& obj) {
        TagInfo tag;
        tag.name = obj.value("name"_str).toString();
        tag.count = obj.value("count"_str).toInt(0);
        tag.id = obj.value("id"_str).toString();
        tag.disambiguation = obj.value("disambiguation"_str).toString();
        return tag;
    }

    Artist parseArtist(const QJsonObject& obj) {
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

    ArtistCredit parseArtistCredit(const QJsonObject& obj) {
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
        rec.lengthMs = rootObj.value("length"_str).toInt(0);
        rec.firstReleaseDate = rootObj.value("first-release-date"_str).toString();

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

        if (rootObj.contains("releases"_str) && rootObj["releases"_str].isArray()) {
            QJsonArray rels = rootObj["releases"_str].toArray();
            for (const auto& v : rels) {
                if (v.isObject()) {
                    rec.releases.append(parseRelease(v.toObject()));
                }
            }
        }

        return rec;
    }

    std::optional<QList<TrackInfo>> parseReleaseTracklist(const QByteArray& json, const QList<Release> &releases) {
        QJsonParseError err{};
        const auto doc = QJsonDocument::fromJson(json, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) 
            return std::nullopt;

        const auto obj = doc.object();
        const auto mediaArr = obj.value("media"_str).toArray();
        QList<TrackInfo> out;

        for (const auto& m : mediaArr) {
            const auto mediaObj = m.toObject();
            const int disc = mediaObj.value("position"_str).toInt(1);
            const auto tracks = mediaObj.value("tracks"_str).toArray();
            for (const auto& t : tracks) {
                const auto tr = t.toObject();
                TrackInfo ti;
                ti.disc = disc;
                ti.trackNo = tr.value("position"_str).toInt(0);
                ti.title = tr.value("title"_str).toString();
                ti.lengthMs = tr.contains("length"_str) ? tr.value("length"_str).toInt(-1) : -1;
                ti.releases = releases;

                const auto recObj = tr.value("recording"_str).toObject();
                ti.recordingId = recObj.value("id"_str).toString();

                // artist-credits 可能在 track 或 recording 上，這裡先取 track 上的
                const auto acArr = tr.value("artist-credit"_str).toArray();
                for (const auto& ac : acArr) {
                    const auto acObj = ac.toObject();
                    const auto name = acObj.value("name"_str).toString();
                    if (!name.isEmpty()) ti.artistCredits.append(name);
                }
                if (ti.artistCredits.isEmpty()) {
                    // 備援：從 recording 內取
                    const auto racArr = recObj.value("artist-credit"_str).toArray();
                    for (const auto& ac : racArr) {
                        const auto acObj = ac.toObject();
                        const auto name = acObj.value("name"_str).toString();
                        if (!name.isEmpty()) 
                            ti.artistCredits.append(name);
                    }
                }

                out.append(std::move(ti));
            }
        }

        if (out.isEmpty()) 
            return std::nullopt;
        return out;
    }
}