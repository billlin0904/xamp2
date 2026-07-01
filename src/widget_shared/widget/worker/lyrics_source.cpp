//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <widget/worker/lyrics_source.h>

#include <algorithm>
#include <sstream>

#include <base/exception.h>
#include <base/logger.h>
#include <widget/krcparser.h>
#include <widget/lrcparser.h>
#include <widget/neteaseparser.h>
#include <widget/util/str_util.h>

namespace {

    QString buildSearchText(const PlayListEntity& keyword) {
        QStringList parts;
        if (!keyword.title.isEmpty()) {
            parts << keyword.title;
        }
        if (!keyword.artist.isEmpty()) {
            parts << keyword.artist;
        }
        return parts.join(" "_str).trimmed();
    }

    template <typename T>
    void keepFirst(QList<T>& values, qsizetype count) {
        if (values.size() > count) {
            values.erase(values.begin() + count, values.end());
        }
    }

    QSharedPointer<ILrcParser> makeLrcParser(const QString& text) {
        if (text.trimmed().isEmpty()) {
            return {};
        }

        auto parser = QSharedPointer<LrcParser>::create();
        std::wistringstream stream(text.toStdWString());
        if (!parser->parse(stream) || parser->size() == 0) {
            return {};
        }
        return parser;
    }

    QSharedPointer<ILrcParser> makeKrcParser(const QByteArray& data) {
        auto parser = QSharedPointer<KrcParser>::create();
        if (!parser->parse(reinterpret_cast<const uint8_t*>(data.constData()),
            static_cast<size_t>(data.size()))) {
            return {};
        }
        return parser;
    }

    InfoItem toInfoItem(const Candidate& candidate) {
        InfoItem info;
        info.albumName = candidate.albumName;
        info.singername = candidate.singer;
        info.songname = candidate.song;
        info.hash = candidate.id;
        return info;
    }

    Candidate toCandidate(const NeteaseSong& song) {
        Candidate candidate;
        candidate.id = QString::number(song.id);
        candidate.song = song.name;
        candidate.albumName = song.album.name;

        QStringList artists;
        for (const auto& artist : song.artists) {
            if (!artist.name.isEmpty()) {
                artists << artist.name;
            }
        }
        candidate.singer = artists.join("/"_str);
        return candidate;
    }

    InfoItem toInfoItem(const NeteaseSong& song) {
        InfoItem info;
        info.songname = song.name;
        if (!song.artists.isEmpty()) {
            info.singername = song.artists[0].name;
        }
        info.albumName = song.album.name;
        info.duration = static_cast<int>(song.duration);
        info.hash = QString::number(song.id);
        return info;
    }

    Candidate toCandidate(const InfoItem& info) {
        Candidate candidate;
        candidate.albumName = info.albumName;
        candidate.singer = info.singername;
        candidate.song = info.songname;
        candidate.id = info.hash;
        return candidate;
    }

    class EmptyLyricsSource final : public ILyricsSource {
    public:
        explicit EmptyLyricsSource(QString source_name)
            : source_name_(std::move(source_name)) {
        }

        QString name() const override {
            return source_name_;
        }

        QCoro::Task<QList<Candidate>> search(const PlayListEntity&) override {
            co_return QList<Candidate>{};
        }

        QCoro::Task<SearchLyricsResult> lookup(const Candidate& candidate) override {
            SearchLyricsResult result;
            result.source_name = source_name_;
            result.info = toInfoItem(candidate);
            co_return result;
        }

    private:
        QString source_name_;
    };

    class NeteaseLyricsSource final : public ILyricsSource {
    public:
        NeteaseLyricsSource(QNetworkAccessManager* nam, QObject* parent)
            : http_client_(nam, QString(), parent) {
        }

        QString name() const override {
            return "Netease"_str;
        }

        QCoro::Task<QList<Candidate>> search(const PlayListEntity& keyword) override {
            const auto search_text = buildSearchText(keyword);
            if (search_text.isEmpty()) {
                co_return QList<Candidate>{};
            }

            http_client_.setUrl("http://music.163.com/api/search/get"_str);
            http_client_.param("s"_str, search_text)
                .param("limit"_str, 20)
                .param("offset"_str, 0)
                .param("type"_str, 1)
                .addAcceptJsonHeader();
            const auto content = co_await http_client_.get();

            QList<Candidate> candidates;
            const auto songs = parseNeteaseSong(content);
            if (!songs) {
                co_return candidates;
            }

            constexpr auto kMaxNeteaseDownload = 10;
            candidates.reserve(songs->size());
            for (const auto& song : *songs) {
                auto candidate = toCandidate(song);
                candidate.accesskey = QString::number(song.duration);
                candidates.push_back(std::move(candidate));
            }
            keepFirst(candidates, kMaxNeteaseDownload);
            co_return candidates;
        }

        QCoro::Task<SearchLyricsResult> lookup(const Candidate& candidate) override {
            SearchLyricsResult result;
            result.source_name = name();
            result.info = toInfoItem(candidate);
            result.info.duration = candidate.accesskey.toInt();

            if (candidate.id.isEmpty()) {
                co_return result;
            }

            http_client_.setUrl("http://music.163.com/api/song/lyric"_str);
            http_client_.param("id"_str, candidate.id)
                .param("lv"_str, -1)
                .param("kv"_str, -1)
                .param("tv"_str, -1)
                .addAcceptJsonHeader();
            const auto content = co_await http_client_.get();

            const auto lyric = parseNeteaseLyric(content);
            if (!lyric) {
                co_return result;
            }

            if (auto parser = makeLrcParser(lyric->lrc.lyric)) {
                LyricsParser lyrics_parser;
                lyrics_parser.candidate = candidate;
                lyrics_parser.content = lyric->lrc.lyric.toUtf8();
                lyrics_parser.parser = std::move(parser);
                result.parsers.push_back(std::move(lyrics_parser));
            }

            if (auto parser = makeLrcParser(lyric->tlyric.lyric)) {
                LyricsParser lyrics_parser;
                lyrics_parser.candidate = candidate;
                lyrics_parser.content = lyric->tlyric.lyric.toUtf8();
                lyrics_parser.parser = std::move(parser);
                result.parsers.push_back(std::move(lyrics_parser));
            }

            co_return result;
        }

    private:
        http::HttpClient http_client_;
    };

    class KugouLyricsSource final : public ILyricsSource {
    public:
        KugouLyricsSource(QNetworkAccessManager* nam, QObject* parent)
            : http_client_(nam, QString(), parent) {
        }

        QString name() const override {
            return "Kugou"_str;
        }

        QCoro::Task<QList<Candidate>> search(const PlayListEntity& keyword) override {
            const auto search_text = buildSearchText(keyword);
            if (search_text.isEmpty()) {
                co_return QList<Candidate>{};
            }

            http_client_.setUrl("http://mobilecdn.kugou.com/api/v3/search/song"_str);
            http_client_.param("format"_str, "json"_str)
                .param("keyword"_str, search_text)
                .addAcceptJsonHeader();
            const auto content = co_await http_client_.get();

            auto infos = parseInfoData(content);
            constexpr auto kMaxKugouDownload = 10;
            keepFirst(infos, kMaxKugouDownload);

            QList<Candidate> candidates;
            candidates.reserve(infos.size());
            for (const auto& info : infos) {
                candidates.push_back(toCandidate(info));
            }
            co_return candidates;
        }

        QCoro::Task<SearchLyricsResult> lookup(const Candidate& candidate) override {
            SearchLyricsResult result;
            result.source_name = name();
            result.info = toInfoItem(candidate);

            if (candidate.id.isEmpty()) {
                co_return result;
            }

            http_client_.setUrl("http://krcs.kugou.com/search"_str);
            http_client_.param("ver"_str, "1"_str)
                .param("man"_str, "yes"_str)
                .param("client"_str, "mobi"_str)
                .param("hash"_str, candidate.id)
                .param("album_audio_id"_str, ""_str)
                .addAcceptJsonHeader();

            const auto search_content = co_await http_client_.get();
            auto download_candidates = parseCandidatesFromJson(search_content);
            for (auto& download_candidate : download_candidates) {
                http_client_.setUrl("http://lyrics.kugou.com/download"_str);
                http_client_.param("ver"_str, "1"_str)
                    .param("client"_str, "pc"_str)
                    .param("id"_str, download_candidate.id)
                    .param("accesskey"_str, download_candidate.accesskey)
                    .param("fmt"_str, "krc"_str)
                    .param("charset"_str, "utf8"_str)
                    .addAcceptJsonHeader();

                const auto krc_data = co_await http_client_.get();
                const auto krc_content = parseKrcContent(krc_data);
                if (!krc_content) {
                    continue;
                }

                try {
                    auto parser = makeKrcParser(krc_content->decodedContent);
                    if (!parser) {
                        continue;
                    }

                    download_candidate.albumName = candidate.albumName;
                    LyricsParser lyrics_parser;
                    lyrics_parser.candidate = download_candidate;
                    lyrics_parser.content = krc_content->decodedContent;
                    lyrics_parser.parser = std::move(parser);
                    result.parsers.push_back(std::move(lyrics_parser));
                }
                catch (const Exception& e) {
                    XAMP_LOG_ERROR(e.getErrorMessage());
                }
                catch (const std::exception& e) {
                    XAMP_LOG_ERROR(e.what());
                }
            }

            co_return result;
        }

    private:
        http::HttpClient http_client_;
    };
}

std::unique_ptr<ILyricsSource> makeNeteaseLyricsSource(QNetworkAccessManager* nam, QObject* parent) {
    return std::make_unique<NeteaseLyricsSource>(nam, parent);
}

std::unique_ptr<ILyricsSource> makeQQMusicLyricsSource(QNetworkAccessManager*, QObject*) {
    return std::make_unique<EmptyLyricsSource>("QQMusic"_str);
}

std::unique_ptr<ILyricsSource> makeKugouLyricsSource(QNetworkAccessManager* nam, QObject* parent) {
    return std::make_unique<KugouLyricsSource>(nam, parent);
}
