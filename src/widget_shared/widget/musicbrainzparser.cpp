#include <widget/musicbrainzparser.h>
#include <widget/util/str_util.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QDebug>
#include <QRegularExpression>
#include <QVector>

#include <algorithm>
#include <cmath>

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
    namespace {
        constexpr double kLengthScoreThresholdMs = 30000.0;

        struct WeightedPart {
            double value = 0;
            double weight = 0;
        };

        QString normalizedText(const QString& text) {
            static const QRegularExpression nonAlnum(QStringLiteral("[^\\p{L}\\p{N}]"));
            auto normalized = text.toCaseFolded();
            normalized.remove(nonAlnum);
            return normalized.isEmpty() ? text.toCaseFolded() : normalized;
        }

        QStringList splitWords(const QString& text) {
            static const QRegularExpression nonAlnum(QStringLiteral("[^\\p{L}\\p{N}]+"));
            return text.toCaseFolded().split(nonAlnum, Qt::SkipEmptyParts);
        }

        int levenshteinDistance(const QString& a, const QString& b) {
            if (a == b) {
                return 0;
            }
            if (a.isEmpty()) {
                return b.size();
            }
            if (b.isEmpty()) {
                return a.size();
            }

            QVector<int> previous(b.size() + 1);
            QVector<int> current(b.size() + 1);
            for (int j = 0; j <= b.size(); ++j) {
                previous[j] = j;
            }
            for (int i = 1; i <= a.size(); ++i) {
                current[0] = i;
                for (int j = 1; j <= b.size(); ++j) {
                    const auto substitution = previous[j - 1] + (a[i - 1] == b[j - 1] ? 0 : 1);
                    current[j] = std::min({ previous[j] + 1, current[j - 1] + 1, substitution });
                }
                previous.swap(current);
            }
            return previous[b.size()];
        }

        double wordSimilarity(const QString& a, const QString& b) {
            const auto left = normalizedText(a);
            const auto right = normalizedText(b);
            if (left.isEmpty() || right.isEmpty()) {
                return 0;
            }
            if (left == right) {
                return 1;
            }
            const auto maxLen = std::max(left.size(), right.size());
            if (maxLen == 0) {
                return 0;
            }
            return std::max(0.0, 1.0 - static_cast<double>(levenshteinDistance(left, right)) / maxLen);
        }

        double textSimilarity(const QString& a, const QString& b) {
            if (a.isEmpty() || b.isEmpty()) {
                return 0;
            }
            if (a == b) {
                return 1;
            }

            auto leftWords = splitWords(a);
            auto rightWords = splitWords(b);
            if (leftWords.isEmpty() || rightWords.isEmpty()) {
                return 0;
            }
            if (leftWords.size() > rightWords.size()) {
                std::swap(leftWords, rightWords);
            }

            double score = 0;
            for (const auto& left : leftWords) {
                double best = 0;
                int bestIndex = -1;
                for (int i = 0; i < rightWords.size(); ++i) {
                    const auto current = wordSimilarity(left, rightWords[i]);
                    if (current > best) {
                        best = current;
                        bestIndex = i;
                    }
                }
                score += best;
                if (best > 0.6 && bestIndex >= 0) {
                    rightWords.removeAt(bestIndex);
                }
            }

            return score / (leftWords.size() + rightWords.size() * 0.4);
        }

        double weightedAverage(const QList<WeightedPart>& parts) {
            double total = 0;
            double sum = 0;
            for (const auto& part : parts) {
                if (part.weight <= 0) {
                    continue;
                }
                const auto value = std::clamp(part.value, 0.0, 1.0);
                total += part.weight;
                sum += value * part.weight;
            }
            return total > 0 ? sum / total : 0;
        }

        double trackCountScore(int actual, int expected) {
            if (actual <= 0 || expected <= 0) {
                return 0;
            }
            if (actual > expected) {
                return 0;
            }
            return actual < expected ? 0.3 : 1.0;
        }

        int yearFromDate(const QString& date) {
            bool ok = false;
            const auto year = date.left(4).toInt(&ok);
            return ok ? year : 0;
        }

        double dateScore(const QString& metadataDate, const QString& releaseDate) {
            if (releaseDate.isEmpty()) {
                return 0.25;
            }
            if (metadataDate.isEmpty()) {
                return 0.65;
            }
            if (metadataDate == releaseDate) {
                return 1;
            }
            const auto metadataYear = yearFromDate(metadataDate);
            const auto releaseYear = yearFromDate(releaseDate);
            if (metadataYear <= 0 || releaseYear <= 0) {
                return 0;
            }
            if (metadataYear == releaseYear) {
                return 0.95;
            }
            return std::abs(metadataYear - releaseYear) <= 2 ? 0.85 : 0;
        }

        QString artistCreditName(const QList<ArtistCredit>& credits) {
            QStringList names;
            for (const auto& credit : credits) {
                if (!credit.name.isEmpty()) {
                    names.append(credit.name);
                }
                else if (!credit.artist.name.isEmpty()) {
                    names.append(credit.artist.name);
                }
            }
            return names.join(QString());
        }

        QString trackArtistName(const TrackInfo& track) {
            return track.artistCredits.join(QStringLiteral(", "));
        }

        double mbScoreFactor(double score) {
            if (score <= 0) {
                return 1;
            }
            return score > 1 ? std::clamp(score / 100.0, 0.0, 1.0) : std::clamp(score, 0.0, 1.0);
        }

        double releaseTypeScore(const ReleaseGroup& group) {
            if (group.primaryType.compare(QStringLiteral("Album"), Qt::CaseInsensitive) == 0) {
                return 1;
            }
            if (group.primaryType.compare(QStringLiteral("EP"), Qt::CaseInsensitive) == 0) {
                return 0.8;
            }
            if (group.primaryType.compare(QStringLiteral("Single"), Qt::CaseInsensitive) == 0) {
                return 0.6;
            }
            return group.primaryType.isEmpty() ? 0.5 : 0.7;
        }
    }

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

    ArtistCredit parseArtistCredit(const QJsonObject& obj);

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
        r.mbSearchScore = obj.value("score"_str).toDouble(0);
        r.trackCount = obj.value("track-count"_str).toInt(0);

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
        if (obj.contains("artist-credit"_str) && obj["artist-credit"_str].isArray()) {
            for (const auto& v : obj["artist-credit"_str].toArray()) {
                if (v.isObject()) {
                    r.artistCredits.append(parseArtistCredit(v.toObject()));
                }
            }
        }
        if (obj.contains("media"_str) && obj["media"_str].isArray()) {
            int mediaTrackCount = 0;
            for (const auto& v : obj["media"_str].toArray()) {
                if (!v.isObject()) {
                    continue;
                }
                const auto media = v.toObject();
                const auto format = media.value("format"_str).toString();
                if (!format.isEmpty()) {
                    r.mediaFormats.append(format);
                }
                mediaTrackCount += media.value("track-count"_str).toInt(0);
            }
            if (r.trackCount <= 0) {
                r.trackCount = mediaTrackCount;
            }
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

    RootRecording parseRootRecordingObject(const QJsonObject& rootObj) {
        RootRecording rec;

        // 1) 基本欄位
        rec.id = rootObj.value("id"_str).toString();
        rec.title = rootObj.value("title"_str).toString();
        rec.video = rootObj.value("video"_str).toBool(false);
        rec.disambiguation = rootObj.value("disambiguation"_str).toString();
        rec.lengthMs = rootObj.value("length"_str).toInt(0);
        rec.mbSearchScore = rootObj.value("score"_str).toDouble(0);
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

    std::expected<RootRecording, ParserError> parseRootRecording(const QString& jsonText) {
        // 將 JSON 字串轉成 QJsonDocument
        QJsonDocument doc = QJsonDocument::fromJson(jsonText.toUtf8());
        if (doc.isNull() || !doc.isObject()) {
            return std::unexpected(ParserError::PARSE_ERROR_JSON_ERROR);
        }

        return parseRootRecordingObject(doc.object());
    }

    std::expected<QList<RootRecording>, ParserError> parseRootRecordingList(const QString& jsonText) {
        QJsonDocument doc = QJsonDocument::fromJson(jsonText.toUtf8());
        if (doc.isNull() || !doc.isObject()) {
            return std::unexpected(ParserError::PARSE_ERROR_JSON_ERROR);
        }

        QList<RootRecording> recordings;
        const auto rootObj = doc.object();
        const auto recordingsValue = rootObj.value("recordings"_str);
        if (!recordingsValue.isArray()) {
            return std::unexpected(ParserError::PARSE_ERROR_JSON_ERROR);
        }

        for (const auto& value : recordingsValue.toArray()) {
            if (value.isObject()) {
                recordings.append(parseRootRecordingObject(value.toObject()));
            }
        }
        return recordings;
    }

    std::expected<QList<Release>, ParserError> parseReleaseList(const QString& jsonText) {
        QJsonDocument doc = QJsonDocument::fromJson(jsonText.toUtf8());
        if (doc.isNull() || !doc.isObject()) {
            return std::unexpected(ParserError::PARSE_ERROR_JSON_ERROR);
        }

        QList<Release> releases;
        const auto rootObj = doc.object();
        const auto releasesValue = rootObj.value("releases"_str);
        if (!releasesValue.isArray()) {
            return std::unexpected(ParserError::PARSE_ERROR_JSON_ERROR);
        }

        for (const auto& value : releasesValue.toArray()) {
            if (value.isObject()) {
                releases.append(parseRelease(value.toObject()));
            }
        }
        return releases;
    }

    std::optional<QList<TrackInfo>> parseReleaseTracklist(const QByteArray& json, const QList<Release> &releases) {
        QJsonParseError err{};
        const auto doc = QJsonDocument::fromJson(json, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) 
            return std::nullopt;

        const auto obj = doc.object();
        auto trackReleases = releases;
        auto release = parseRelease(obj);
        if (!release.id.isEmpty()) {
            if (!releases.isEmpty() && releases.front().id == release.id) {
                if (release.mbSearchScore <= 0) {
                    release.mbSearchScore = releases.front().mbSearchScore;
                }
                if (release.releaseGroup.primaryType.isEmpty()) {
                    release.releaseGroup = releases.front().releaseGroup;
                }
            }
            trackReleases = { release };
        }

        const auto mediaArr = obj.value("media"_str).toArray();
        QList<TrackInfo> out;

        for (const auto& m : mediaArr) {
            const auto mediaObj = m.toObject();
            const int disc = mediaObj.value("position"_str).toInt(1);
            const auto tracks = mediaObj.value("tracks"_str).toArray();
            for (const auto& t : tracks) {
                const auto tr = t.toObject();
                TrackInfo ti;
                ti.id = tr.value("id"_str).toString();
                ti.disc = disc;
                ti.trackNo = tr.value("position"_str).toInt(0);
                ti.title = tr.value("title"_str).toString();
                ti.lengthMs = tr.contains("length"_str) ? tr.value("length"_str).toInt(-1) : -1;
                ti.releases = trackReleases;

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

    double lengthScore(int a, int b) {
        if (a <= 0 || b <= 0) {
            return 0;
        }
        return 1.0 - std::min(std::abs(a - b), static_cast<int>(kLengthScoreThresholdMs)) / kLengthScoreThresholdMs;
    }

    double compareToRelease(const FileMeta& meta, const Release& release) {
        QList<WeightedPart> parts;
        if (!meta.album.isEmpty() && !release.title.isEmpty()) {
            parts.append({ textSimilarity(meta.album, release.title), 17 });
        }

        const auto albumArtist = meta.albumArtist.isEmpty() ? meta.artist : meta.albumArtist;
        const auto releaseArtist = artistCreditName(release.artistCredits);
        if (!albumArtist.isEmpty() && !releaseArtist.isEmpty()) {
            parts.append({ textSimilarity(albumArtist, releaseArtist), 6 });
        }

        const auto totalTracks = meta.totalAlbumTracks > 0 ? meta.totalAlbumTracks : meta.totalTracks;
        if (totalTracks > 0 && release.trackCount > 0) {
            parts.append({ trackCountScore(totalTracks, release.trackCount), 5 });
        }

        parts.append({ dateScore(meta.date, release.date), 4 });
        parts.append({ releaseTypeScore(release.releaseGroup), 10 });

        if (!meta.barcode.isEmpty()) {
            if (release.barcode.isEmpty()) {
                parts.append({ 0.5, 6 });
            }
            else {
                parts.append({ meta.barcode == release.barcode ? 1.0 : 0.0, 6 });
            }
        }

        return weightedAverage(parts) * mbScoreFactor(release.mbSearchScore);
    }

    double compareToRecording(const FileMeta& meta, const RootRecording& recording) {
        QList<WeightedPart> parts;
        if (!meta.title.isEmpty() && !recording.title.isEmpty()) {
            parts.append({ textSimilarity(meta.title, recording.title), 13 });
        }
        const auto recordingArtist = artistCreditName(recording.artistCredits);
        if (!meta.artist.isEmpty() && !recordingArtist.isEmpty()) {
            parts.append({ textSimilarity(meta.artist, recordingArtist), 4 });
        }
        if (meta.lengthMs > 0 && recording.lengthMs > 0) {
            parts.append({ lengthScore(meta.lengthMs, recording.lengthMs), 10 });
        }
        parts.append({ meta.video == recording.video ? 1.0 : 0.0, 2 });

        double bestReleaseScore = 0;
        for (const auto& release : recording.releases) {
            bestReleaseScore = std::max(bestReleaseScore, compareToRelease(meta, release));
        }
        if (bestReleaseScore > 0) {
            parts.append({ bestReleaseScore, 33 });
        }

        return weightedAverage(parts) * mbScoreFactor(recording.mbSearchScore);
    }

    double compareToTrack(const FileMeta& meta, const TrackInfo& track) {
        QList<WeightedPart> parts;
        if (!meta.title.isEmpty() && !track.title.isEmpty()) {
            parts.append({ textSimilarity(meta.title, track.title), 22 });
        }
        const auto artist = trackArtistName(track);
        if (!meta.artist.isEmpty() && !artist.isEmpty()) {
            parts.append({ textSimilarity(meta.artist, artist), 6 });
        }
        if (meta.lengthMs > 0 && track.lengthMs > 0) {
            parts.append({ lengthScore(meta.lengthMs, track.lengthMs), 8 });
        }
        if (meta.trackNumber > 0 && track.trackNo > 0) {
            parts.append({ meta.trackNumber == track.trackNo ? 1.0 : 0.0, 6 });
        }
        if (meta.discNumber > 0 && track.disc > 0) {
            parts.append({ meta.discNumber == track.disc ? 1.0 : 0.0, 5 });
        }

        if (!track.releases.isEmpty()) {
            parts.append({ compareToRelease(meta, track.releases.front()), 12 });
        }

        return weightedAverage(parts);
    }
}
