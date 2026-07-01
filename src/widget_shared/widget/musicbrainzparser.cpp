#include <widget/musicbrainzparser.h>
#include <widget/util/str_util.h>

#include <QString>
#include <QDebug>
#include <QRegularExpression>
#include <QVector>

#include <simdjson.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string_view>

namespace musicbrain {
    namespace {
        constexpr double kLengthScoreThresholdMs = 30000.0;

        struct WeightedPart {
            double value = 0;
            double weight = 0;
        };

        using JsonArray = simdjson::dom::array;
        using JsonElement = simdjson::dom::element;
        using JsonObject = simdjson::dom::object;

        QString toQString(std::string_view value) {
            return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
        }

        std::optional<JsonObject> asObject(const JsonElement& element) {
            JsonObject object;
            if (element.get(object)) {
                return std::nullopt;
            }
            return object;
        }

        std::optional<JsonArray> asArray(const JsonElement& element) {
            JsonArray array;
            if (element.get(array)) {
                return std::nullopt;
            }
            return array;
        }

        std::optional<JsonElement> field(const JsonObject& object, std::string_view name) {
            auto result = object[name];
            if (result.error()) {
                return std::nullopt;
            }
            return result.value_unsafe();
        }

        std::optional<JsonObject> objectField(const JsonObject& object, std::string_view name) {
            const auto value = field(object, name);
            return value ? asObject(*value) : std::nullopt;
        }

        std::optional<JsonArray> arrayField(const JsonObject& object, std::string_view name) {
            const auto value = field(object, name);
            return value ? asArray(*value) : std::nullopt;
        }

        QString stringField(const JsonObject& object, std::string_view name) {
            const auto value = field(object, name);
            if (!value) {
                return {};
            }

            std::string_view text;
            if (value->get(text)) {
                return {};
            }
            return toQString(text);
        }

        int intField(const JsonObject& object, std::string_view name, int fallback = 0) {
            const auto value = field(object, name);
            if (!value) {
                return fallback;
            }

            int64_t signedValue = 0;
            if (!value->get(signedValue)) {
                return static_cast<int>(signedValue);
            }

            uint64_t unsignedValue = 0;
            if (!value->get(unsignedValue)) {
                return static_cast<int>(unsignedValue);
            }

            double doubleValue = 0;
            if (!value->get(doubleValue)) {
                return static_cast<int>(doubleValue);
            }

            return fallback;
        }

        double doubleField(const JsonObject& object, std::string_view name, double fallback = 0) {
            const auto value = field(object, name);
            if (!value) {
                return fallback;
            }

            double doubleValue = 0;
            if (!value->get(doubleValue)) {
                return doubleValue;
            }

            int64_t signedValue = 0;
            if (!value->get(signedValue)) {
                return static_cast<double>(signedValue);
            }

            uint64_t unsignedValue = 0;
            if (!value->get(unsignedValue)) {
                return static_cast<double>(unsignedValue);
            }

            return fallback;
        }

        bool boolField(const JsonObject& object, std::string_view name, bool fallback = false) {
            const auto value = field(object, name);
            if (!value) {
                return fallback;
            }

            bool boolValue = false;
            return value->get(boolValue) ? fallback : boolValue;
        }

        QStringList stringArrayField(const JsonObject& object, std::string_view name) {
            QStringList values;
            const auto array = arrayField(object, name);
            if (!array) {
                return values;
            }

            for (const auto value : *array) {
                std::string_view text;
                if (!value.get(text)) {
                    values.append(toQString(text));
                }
            }
            return values;
        }

        std::optional<JsonObject> parseJsonObject(const QByteArray& json, simdjson::dom::parser& parser) {
            simdjson::padded_string padded(json.constData(), static_cast<size_t>(json.size()));
            JsonElement root;
            if (parser.parse(padded).get(root)) {
                return std::nullopt;
            }
            return asObject(root);
        }

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

    TextRepresentation parseTextRep(const JsonObject& obj) {
        TextRepresentation tr;
        tr.language = stringField(obj, "language");
        tr.script = stringField(obj, "script");
        return tr;
    }

    Area parseArea(const JsonObject& obj) {
        Area a;
        a.id = stringField(obj, "id");
        a.name = stringField(obj, "name");
        a.sortName = stringField(obj, "sort-name");
        a.type = stringField(obj, "type");
        a.typeId = stringField(obj, "type-id");
        a.disambiguation = stringField(obj, "disambiguation");
        a.iso3166_1 = stringArrayField(obj, "iso-3166-1-codes");
        return a;
    }

    ReleaseEvent parseReleaseEvent(const JsonObject& obj) {
        ReleaseEvent e;
        e.date = stringField(obj, "date");
        if (const auto area = objectField(obj, "area")) {
            e.area = parseArea(*area);
        }
        return e;
    }

    ReleaseGroup parseReleaseGroup(const JsonObject& obj) {
        ReleaseGroup g;
        g.id = stringField(obj, "id");
        g.title = stringField(obj, "title");
        g.primaryType = stringField(obj, "primary-type");
        g.primaryTypeId = stringField(obj, "primary-type-id");
        g.firstReleaseDate = stringField(obj, "first-release-date");
        g.disambiguation = stringField(obj, "disambiguation");
        g.secondaryTypes = stringArrayField(obj, "secondary-types");
        g.secondaryTypeIds = stringArrayField(obj, "secondary-type-ids");
        return g;
    }

    ArtistCredit parseArtistCredit(const JsonObject& obj);

    Release parseRelease(const JsonObject& obj) {
        Release r;
        r.id = stringField(obj, "id");
        r.title = stringField(obj, "title");
        r.status = stringField(obj, "status");
        r.statusId = stringField(obj, "status-id");
        r.country = stringField(obj, "country");
        r.barcode = stringField(obj, "barcode");
        r.packaging = stringField(obj, "packaging");
        r.packagingId = stringField(obj, "packaging-id");
        r.disambiguation = stringField(obj, "disambiguation");
        r.quality = stringField(obj, "quality");
        r.date = stringField(obj, "date");
        r.mbSearchScore = doubleField(obj, "score");
        r.trackCount = intField(obj, "track-count");

        if (const auto textRep = objectField(obj, "text-representation")) {
            r.textRep = parseTextRep(*textRep);
        }
        if (const auto events = arrayField(obj, "release-events")) {
            for (const auto value : *events) {
                if (const auto event = asObject(value)) {
                    r.events.append(parseReleaseEvent(*event));
                }
            }
        }
        if (const auto releaseGroup = objectField(obj, "release-group")) {
            r.releaseGroup = parseReleaseGroup(*releaseGroup);
        }
        if (const auto credits = arrayField(obj, "artist-credit")) {
            for (const auto value : *credits) {
                if (const auto credit = asObject(value)) {
                    r.artistCredits.append(parseArtistCredit(*credit));
                }
            }
        }
        if (const auto mediaArray = arrayField(obj, "media")) {
            int mediaTrackCount = 0;
            for (const auto value : *mediaArray) {
                const auto media = asObject(value);
                if (!media) {
                    continue;
                }
                const auto format = stringField(*media, "format");
                if (!format.isEmpty()) {
                    r.mediaFormats.append(format);
                }
                mediaTrackCount += intField(*media, "track-count");
            }
            if (r.trackCount <= 0) {
                r.trackCount = mediaTrackCount;
            }
        }
        return r;
    }

    TagInfo parseTagInfo(const JsonObject& obj) {
        TagInfo tag;
        tag.name = stringField(obj, "name");
        tag.count = intField(obj, "count");
        tag.id = stringField(obj, "id");
        tag.disambiguation = stringField(obj, "disambiguation");
        return tag;
    }

    Artist parseArtist(const JsonObject& obj) {
        Artist a;
        a.id = stringField(obj, "id");
        a.name = stringField(obj, "name");
        a.sortName = stringField(obj, "sort-name");
        a.disambiguation = stringField(obj, "disambiguation");
        a.country = stringField(obj, "country");
        a.type = stringField(obj, "type");
        a.typeId = stringField(obj, "type-id");

        if (const auto tags = arrayField(obj, "tags")) {
            for (const auto value : *tags) {
                if (const auto tag = asObject(value)) {
                    a.tags.append(parseTagInfo(*tag));
                }
            }
        }

        if (const auto genres = arrayField(obj, "genres")) {
            for (const auto value : *genres) {
                if (const auto genre = asObject(value)) {
                    a.genres.append(parseTagInfo(*genre));
                }
            }
        }

        return a;
    }

    ArtistCredit parseArtistCredit(const JsonObject& obj) {
        ArtistCredit ac;
        ac.joinPhrase = stringField(obj, "joinphrase");
        ac.name = stringField(obj, "name");

        if (const auto artist = objectField(obj, "artist")) {
            ac.artist = parseArtist(*artist);
        }

        return ac;
    }    

    RootRecording parseRootRecordingObject(const JsonObject& rootObj) {
        RootRecording rec;

        rec.id = stringField(rootObj, "id");
        rec.title = stringField(rootObj, "title");
        rec.video = boolField(rootObj, "video");
        rec.disambiguation = stringField(rootObj, "disambiguation");
        rec.lengthMs = intField(rootObj, "length");
        rec.mbSearchScore = doubleField(rootObj, "score");
        rec.firstReleaseDate = stringField(rootObj, "first-release-date");

        if (const auto credits = arrayField(rootObj, "artist-credit")) {
            for (const auto value : *credits) {
                if (const auto credit = asObject(value)) {
                    rec.artistCredits.append(parseArtistCredit(*credit));
                }
            }
        }

        if (const auto tags = arrayField(rootObj, "tags")) {
            for (const auto value : *tags) {
                if (const auto tag = asObject(value)) {
                    rec.tags.append(parseTagInfo(*tag));
                }
            }
        }

        if (const auto genres = arrayField(rootObj, "genres")) {
            for (const auto value : *genres) {
                if (const auto genre = asObject(value)) {
                    rec.genres.append(parseTagInfo(*genre));
                }
            }
        }

        if (const auto releases = arrayField(rootObj, "releases")) {
            for (const auto value : *releases) {
                if (const auto release = asObject(value)) {
                    rec.releases.append(parseRelease(*release));
                }
            }
        }

        return rec;
    }

    std::expected<RootRecording, ParserError> parseRootRecording(const QString& jsonText) {
        simdjson::dom::parser parser;
        const auto rootObj = parseJsonObject(jsonText.toUtf8(), parser);
        if (!rootObj) {
            return std::unexpected(ParserError::PARSE_ERROR_JSON_ERROR);
        }

        return parseRootRecordingObject(*rootObj);
    }

    std::expected<QList<RootRecording>, ParserError> parseRootRecordingList(const QString& jsonText) {
        simdjson::dom::parser parser;
        const auto rootObj = parseJsonObject(jsonText.toUtf8(), parser);
        if (!rootObj) {
            return std::unexpected(ParserError::PARSE_ERROR_JSON_ERROR);
        }

        QList<RootRecording> recordings;
        const auto recordingsValue = arrayField(*rootObj, "recordings");
        if (!recordingsValue) {
            return std::unexpected(ParserError::PARSE_ERROR_JSON_ERROR);
        }

        for (const auto value : *recordingsValue) {
            if (const auto recording = asObject(value)) {
                recordings.append(parseRootRecordingObject(*recording));
            }
        }
        return recordings;
    }

    std::expected<QList<Release>, ParserError> parseReleaseList(const QString& jsonText) {
        simdjson::dom::parser parser;
        const auto rootObj = parseJsonObject(jsonText.toUtf8(), parser);
        if (!rootObj) {
            return std::unexpected(ParserError::PARSE_ERROR_JSON_ERROR);
        }

        QList<Release> releases;
        const auto releasesValue = arrayField(*rootObj, "releases");
        if (!releasesValue) {
            return std::unexpected(ParserError::PARSE_ERROR_JSON_ERROR);
        }

        for (const auto value : *releasesValue) {
            if (const auto release = asObject(value)) {
                releases.append(parseRelease(*release));
            }
        }
        return releases;
    }

    std::optional<QList<TrackInfo>> parseReleaseTracklist(const QByteArray& json, const QList<Release> &releases) {
        simdjson::dom::parser parser;
        const auto obj = parseJsonObject(json, parser);
        if (!obj) {
            return std::nullopt;
        }

        auto trackReleases = releases;
        auto release = parseRelease(*obj);
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

        const auto mediaArr = arrayField(*obj, "media");
        QList<TrackInfo> out;
        if (!mediaArr) {
            return std::nullopt;
        }

        for (const auto mediaValue : *mediaArr) {
            const auto mediaObj = asObject(mediaValue);
            if (!mediaObj) {
                continue;
            }
            const int disc = intField(*mediaObj, "position", 1);
            const auto tracks = arrayField(*mediaObj, "tracks");
            if (!tracks) {
                continue;
            }
            for (const auto trackValue : *tracks) {
                const auto tr = asObject(trackValue);
                if (!tr) {
                    continue;
                }
                TrackInfo ti;
                ti.id = stringField(*tr, "id");
                ti.disc = disc;
                ti.trackNo = intField(*tr, "position");
                ti.title = stringField(*tr, "title");
                ti.lengthMs = field(*tr, "length") ? intField(*tr, "length", -1) : -1;
                ti.releases = trackReleases;

                const auto recObj = objectField(*tr, "recording");
                if (recObj) {
                    ti.recordingId = stringField(*recObj, "id");
                }

                // artist-credits 可能在 track 或 recording 上，這裡先取 track 上的
                if (const auto acArr = arrayField(*tr, "artist-credit")) {
                    for (const auto acValue : *acArr) {
                        const auto acObj = asObject(acValue);
                        if (!acObj) {
                            continue;
                        }
                        const auto name = stringField(*acObj, "name");
                        if (!name.isEmpty()) {
                            ti.artistCredits.append(name);
                        }
                    }
                }
                if (ti.artistCredits.isEmpty()) {
                    // 備援：從 recording 內取
                    const auto racArr = recObj ? arrayField(*recObj, "artist-credit") : std::nullopt;
                    if (racArr) {
                        for (const auto acValue : *racArr) {
                            const auto acObj = asObject(acValue);
                            if (!acObj) {
                                continue;
                            }
                            const auto name = stringField(*acObj, "name");
                            if (!name.isEmpty()) {
                                ti.artistCredits.append(name);
                            }
                        }
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
