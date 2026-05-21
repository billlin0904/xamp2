#include <widget/worker/backgroundservice.h>

#include <widget/util/ui_util.h>
#include <widget/util/str_util.h>
#include <widget/util/image_util.h>
#include <widget/database.h>
#include <widget/databasefacade.h>
#include <widget/util/mbdiscid_util.h>
#include <widget/chatgpt/waveformwidget.h>
#include <widget/appsettings.h>
#include <widget/widget_shared.h>
#include <widget/imagecache.h>
#include <widget/albumview.h>
#include <widget/krcparser.h>
#include <widget/lrcparser.h>
#include <widget/neteaseparser.h>
#include <widget/util/json_util.h>
#include <widget/util/tag_util.h>

#include <widget/util/read_util.h>
#include <widget/util/colortable.h>
#include <widget/appsettingnames.h>

#include <stream/filestream.h>
#include <stream/bassfilestream.h>
#include <stream/fft.h>
#include <stream/stft.h>
#include <base/logger.h>
#include <base/math.h>

#if defined(Q_OS_WIN)
#include <stream/mbdiscid.h>
#endif

#include <QDir>
#include <QFuture>
#include <QJsonValueRef>

#include <algorithm>
#include <cmath>

namespace {
    XAMP_DECLARE_LOG_NAME(BackgroundService);

    constexpr size_t kFFTSize = 2048 * 2;
    constexpr size_t kHopSize = kFFTSize * 0.5;
    constexpr float kPower2FFSize = kFFTSize * kFFTSize;
    constexpr size_t kFreqBins = (kFFTSize / 2);

    float toDbFromNorm(float p) {
        // p 已是功率（re^2+im^2），保底避免 log(0)
        if (p <= 0.0f) 
            return ColorTable::kMinDb;
        // 若常數很大，建議 kPower2FFSize 用 double 並在此轉回 float
        return 10.0f * log10f_fast(p / kPower2FFSize);
    }

    float getDb(const std::complex<float>& c) {
        return toDbFromNorm(std::norm(c));
    }

    float getRealDb(float r) {
        return toDbFromNorm(r * r);
    }

    void fillSpectrogramColumn(const ColorTable &color_table,
        QImage& chunk_img,
        int col,
        const ComplexValarray& freq_bins) {

        const int h = chunk_img.height();
        const int stride = chunk_img.bytesPerLine();
        const int n = static_cast<int>(freq_bins.size());

        const int num = h - 1;
        const int den = n - 1;

        uchar* base = chunk_img.bits() + col * 3;
        auto toRow = [&](int f) {
            return ((h - 1) - (f * num + den / 2) / den) * stride;
            };

        std::vector<int> row_of_f(n);
        {
            const int num = h - 1, den = n - 1;
            for (int f = 0; f < n; ++f) {
                const int row = toRow(f);
                row_of_f[f] = row;
            }
        }

        for (int f = 0; f < n; ++f) {
            float db;
            if ((f == 0) || (f == n - 1)) {
                db = getRealDb(freq_bins[f].real());
            }
            else {
                db = getDb(freq_bins[f]);
            }

            const QRgb c = color_table[db];

            uchar* pix = base + row_of_f[f];
            pix[0] = qRed(c);
            pix[1] = qGreen(c);
            pix[2] = qBlue(c);
        }
    }

    bool getSamples(ScopedPtr<FileStream>& file_stream, Buffer<float>& buffer) {
        auto retry_count = 0;
        auto is_readable = true;
        auto* bass_file_stream = dynamic_cast<BassFileStream*>(file_stream.get());
        if (!bass_file_stream) {
            return false;
        }
        constexpr auto kMaxRetryCount = 4;
        while (is_readable) {
            buffer.Fill(0.0f);
            auto read_samples = file_stream->GetSamples(buffer.data(),
                buffer.size());
            if (read_samples > 0) {
                break;
            }
            if (retry_count < kMaxRetryCount) {
                if (!bass_file_stream->EndOfStream()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    retry_count++;
                    continue;
                }
            }
            is_readable = false;
        }
        return is_readable;
    }    
}

BackgroundService::BackgroundService()
    : nam_(this)
	, http_client_(&nam_, QString(), this) {
    logger_ = XAMP_LOG_CREATE_LOGGER(BackgroundService);
    thread_pool_ = ThreadPoolBuilder::MakeBackgroundThreadPool();
}

BackgroundService::~BackgroundService() = default;

void BackgroundService::cancelAllJob() {
    stop_source_.request_stop();
}

void BackgroundService::onTranscribeFile(const QString& file_name) {
    http_client_.setUrl("http://127.0.0.1:9090/transcribe_file_krc/"_str);
    http_client_.setTimeout(30000);
    http_client_.postMultipart(file_name)
		.then([this](const auto& content) {
        QSharedPointer<KrcParser> parser(new KrcParser());
        if (parser->parse(reinterpret_cast<const uint8_t*>(content.data()),
            content.size())) {
			emit transcribeFileCompleted(parser);
        }
    });
}

std::tuple<std::shared_ptr<FastIOStream>, Path> 
BackgroundService::makeUniqueFile(const EncodeJob &job,
    const QString& dir_name) {
    std::shared_ptr<FastIOStream> file_writer;
    Path output_path;

    auto file_name = getValidFileName(job.file_.file_name);

    // Ensure file name is unique.
    constexpr auto kMaxRetryTestUniqueFileName = 128;
    auto i = 0;
    for (; i < kMaxRetryTestUniqueFileName; ++i) {
        try {
            QString unique_save_file_name;
            switch (job.type) {
            case EncodeType::ENCODE_AAC:
            case EncodeType::ENCODE_ALAC:
                unique_save_file_name = uniqueFileName(
                    QDir(dir_name),
                    file_name + ".m4a"_str);
                break;
            case EncodeType::ENCODE_PCM:
                unique_save_file_name = uniqueFileName(
                    QDir(dir_name),
                    file_name + ".wav"_str);
                break;
            default:
                return std::make_tuple(nullptr, "");
            }

            auto save_file_name = dir_name
                + "/"_str
                + unique_save_file_name;
            output_path = save_file_name.toStdWString();
            file_writer = std::make_shared<FastIOStream>(output_path, FastIOStream::Mode::ReadWrite);
            break;
        }
        catch (const Exception& e) {
            XAMP_LOG_ERROR(e.GetErrorMessage());
        }
    }
    if (i == kMaxRetryTestUniqueFileName) {
        XAMP_LOG_ERROR("Failed to create unique file name.");
        return std::make_tuple(nullptr, "");
    }
    return std::make_tuple(file_writer, output_path);
}

void BackgroundService::sequenceEncode(const QString& dir_name, QList<EncodeJob> jobs) {
    Q_FOREACH(auto job, jobs) {
		executeEncodeJob(dir_name, job);
    }
}

void BackgroundService::executeEncodeJob(const QString& dir_name, const EncodeJob & job) {
    auto [file_writer, output_path] = makeUniqueFile(
        job,
        dir_name);

    if (file_writer == nullptr || output_path.empty()) {
        emit jobError(job.job_id, tr("executeEncodeJob error"));
        return;
    }

    Path input_path(job.file_.file_path.toStdWString());
    auto stop_token = stop_source_.get_token();

    try {
        auto encoder = StreamFactory::MakeFileEncoder();

        Property config;
        config.Create(FileEncoderConfig::kInputFilePath,
            input_path);
        config.Create(FileEncoderConfig::kOutputFilePath,
            output_path);
        config.Create(FileEncoderConfig::kCodecId,
            job.codec_id.toStdString());
        config.Create(FileEncoderConfig::kBitRate,
            job.bit_rate);

        encoder->Start(config, file_writer);
        encoder->Encode([job, &stop_token, this](auto progress) {
            if (stop_token.stop_requested()) {
                return false;
            }
            if (progress % 10 == 0) {
				//std::this_thread::sleep_for(std::chrono::milliseconds(100));
                emit updateJobProgress(job.job_id, progress);
            }            
            return true;
            });
        file_writer.reset();
        encoder.reset();

        if (stop_token.stop_requested()) {
            Fs::remove(output_path);
            XAMP_LOG_DEBUG("executeEncodeJob job canceled.");
            emit jobError(job.job_id, tr("Canceled"));
			return;
        }

        auto writer = MakeMetadataWriter();
        writer->Open(output_path);
        writer->WriteArtist(job.file_.artist.toStdWString());
        writer->WriteTitle(job.file_.title.toStdWString());
        writer->WriteAlbum(job.file_.album.toStdWString());
        writer->WriteComment(job.file_.comment.toStdWString());
        writer->WriteGenre(job.file_.genre.toStdWString());
        writer->WriteTrack(job.file_.track);
        writer->WriteYear(job.file_.year);

        auto reader = MakeMetadataReader();
        reader->Open(input_path);
        auto cover = tag_util::readEmbeddedCover(*reader);
        if (!cover.isNull()) {
            tag_util::writeEmbeddedCover(*writer, cover);
        }

        emit updateJobProgress(job.job_id, 100);
    }
    catch (const Exception& e) {
        XAMP_LOG_ERROR(e.GetStackTrace());
        emit jobError(job.job_id, tr("executeEncodeJob error"));
    }
    catch (const std::exception& e) {
        XAMP_LOG_ERROR(e.what());
        emit jobError(job.job_id, tr("executeEncodeJob error"));
    }
}

QCoro::Task<std::optional<QByteArray>> BackgroundService::tryFetch(const QString& tag, const QString& release_id, size_t size) {
    //http::HttpClient http_client(&nam_, QString(), this);

    const auto url = (size > 0)
        ? qFormat("https://coverartarchive.org/%1/%2/front-%3").arg(tag).arg(release_id).arg(size)
        : qFormat("https://coverartarchive.org/%1/%2/front").arg(tag).arg(release_id);
    http_client_.setUrl(url);
    http_client_.setHeader("Accept"_str, "image/*"_str);

    auto img = co_await http_client_.download();
    if (!img.isEmpty())
        co_return img;
    co_return std::nullopt;
}

QCoro::Task<std::optional<QByteArray>> BackgroundService::fetchCoverArtByUrl(const QString& tag, const QString& release_id, size_t prefer_size) {
    std::optional<QByteArray> b;
    if (prefer_size > 0) {
        auto result = co_await tryFetch(tag, release_id, prefer_size);
        if (result.has_value()) {
            b = result.value();
        } else {
            result = co_await tryFetch(tag, release_id, 500);
            if (result.has_value()) {
                b = result.value();
            }
        }
    } else {
        auto result = co_await tryFetch(tag, release_id, -1);
        if (result.has_value()) {
            b = result.value();
        }
    }
    if (!b.has_value()) {
        XAMP_LOG_DEBUG("Not found cover art.");
        co_return std::nullopt;
	}
	co_return b;
}

void BackgroundService::parallelEncode(const QString& dir_name, QList<EncodeJob> jobs) {
    auto stop_token = stop_source_.get_token();
    Executor::ParallelForEach(thread_pool_, jobs,
        [this, dir_name](const EncodeJob& job) {
        executeEncodeJob(dir_name, job);
        }, stop_token);
}

void BackgroundService::onAddJobs(const QString& dir_name, const QList<EncodeJob>& jobs) {
    stop_source_ = std::stop_source();

    QList<EncodeJob> parallel_jobs;
	Q_FOREACH(auto job, jobs) {
		if (job.file_.disc_id.isEmpty()) {
			parallel_jobs.push_back(job);
		}
		else {
			sequenceEncode(dir_name, { job });
		}
    }
    parallelEncode(dir_name, parallel_jobs);

    //sequenceEncode(dir_name, jobs);
}

void BackgroundService::cancelRequested() {
    is_stop_ = true;
}

QCoro::Task<SearchLyricsResult> BackgroundService::downloadSingleNeteaseLrc(NeteaseSong info) {
    if (info.artists.isEmpty()) {
        co_return {};
    }

    http::HttpClient http(&nam_, "http://music.163.com/api/song/lyric"_str, this);    
    http.param("id"_str, info.id);
    http.param("lv"_str, -1);
    http.param("kv"_str, -1);
    http.param("tv"_str, -1);
    auto content = co_await http.get();
    //XAMP_LOG_DEBUG("Response: {}", content.toStdString());

	auto netease_lrc = parseNeteaseLyric(content);
    SearchLyricsResult result;
	if (!netease_lrc.has_value()) {
		co_return result;
	}    
    
    QSharedPointer<LrcParser> parser(new LrcParser());
	auto utf8_content = std::wstringstream(netease_lrc->lrc.lyric.toStdWString());
    if (!parser->parse(utf8_content)) {
        co_return result;
    }

    InfoItem info_item;
	info_item.songname = info.name;
	info_item.singername = info.artists[0].name;
	info_item.duration = info.duration;

    LyricsParser lrc_parser;
    
	lrc_parser.parser = parser;
	lrc_parser.candidate.albumName = info.album.name;
	lrc_parser.candidate.song = info.name;
	lrc_parser.candidate.singer = info.artists[0].name;
    lrc_parser.content = netease_lrc->lrc.lyric.toUtf8();

	result.info = info_item;
	result.parsers.append(lrc_parser);

    co_return result;
}

QCoro::Task<QList<SearchLyricsResult>> BackgroundService::downloadNeteaseLrc(QList<NeteaseSong> infos) {
    std::vector<QCoro::Task<SearchLyricsResult>> tasks;
    tasks.reserve(infos.size());

    for (const auto& info : infos) {
        tasks.push_back(downloadSingleNeteaseLrc(info));
    }

    QList<SearchLyricsResult> results;
    results.reserve(infos.size());

    for (auto& task : tasks) {
        auto single_result = co_await task;
        if (!single_result.parsers.empty()) {
            results.push_back(std::move(single_result));
        }
    }
    co_return results;
}

QCoro::Task<SearchLyricsResult> BackgroundService::downloadSingleKlrc(InfoItem info) {
    SearchLyricsResult result;
    result.info = info;

    // 1. 搜尋候選列表
    http::HttpClient http(&nam_, "http://krcs.kugou.com/search"_str, this);
    http.param("ver"_str, "1"_str);
    http.param("man"_str, "yes"_str);
    http.param("client"_str, "mobi"_str);
    http.param("hash"_str, info.hash);
    http.param("album_audio_id"_str, ""_str);
    //http.param("page"_str, "1"_str);
    //http.param("pagesize"_str, "1"_str);

    auto content = co_await http.get();
    auto candidates = parseCandidatesFromJson(content);
    //XAMP_LOG_DEBUG("Found candidates size: {}", candidates.size());

    // 2. 逐一下載 KRC
    for (auto& candidate : candidates) {
        // 建立新的 HttpClient 指向下載 API
        http::HttpClient http_download(&nam_, 
            "http://lyrics.kugou.com/download"_str,
            this);
        http_download.param("ver"_str, "1"_str);
        http_download.param("client"_str, "pc"_str);
        http_download.param("id"_str, candidate.id);
        http_download.param("accesskey"_str, candidate.accesskey);
        http_download.param("fmt"_str, "krc"_str);
        http_download.param("charset"_str, "utf8"_str);

        auto krc_data = co_await http_download.get();

        // 解析
        LyricsParser lrc_parser;
        candidate.albumName = info.albumName;
        lrc_parser.candidate = candidate;

        auto krc_content = parseKrcContent(krc_data);
        if (!krc_content.has_value()) {
            continue;
        }

        QSharedPointer<KrcParser> parser(new KrcParser());
        try {
            if (!parser->parse(
                reinterpret_cast<uint8_t*>(krc_content->decodedContent.data()),
                krc_content->decodedContent.size())) {
                continue;
            }
            lrc_parser.parser = parser;
            lrc_parser.content = krc_content->decodedContent;
            result.parsers.push_back(lrc_parser);
        }        
        catch (const Exception& e) {
            XAMP_LOG_ERROR(e.GetErrorMessage());
        }
        catch (const std::exception& e) {
            XAMP_LOG_ERROR(e.what());
        }
    }

    co_return result;
}

QCoro::Task<QList<SearchLyricsResult>> BackgroundService::downloadKLrc(QList<InfoItem> infos) {
    std::vector<QCoro::Task<SearchLyricsResult>> tasks;
    tasks.reserve(infos.size());

    for (const auto& info : infos) {
        tasks.push_back(downloadSingleKlrc(info));
    }

    QList<SearchLyricsResult> results;
    results.reserve(infos.size());

    for (auto& task : tasks) {
		auto single_result = co_await task;
        if (!single_result.parsers.empty()) {
            results.push_back(std::move(single_result));
        }
    }
    co_return results;
}

QCoro::Task<> BackgroundService::searchKugou(const PlayListEntity& keyword) {
    constexpr auto kMaxDownloadSize = 10;
    const auto search_text = (keyword.title + " "_str + keyword.artist).trimmed();

    http::HttpClient http(&nam_, "http://mobilecdn.kugou.com/api/v3/search/song"_str, this);
    http.param("format"_str, "json"_str);
    http.param("keyword"_str, search_text.isEmpty() ? keyword.title : search_text);
    auto title = keyword.title;
    auto artist = keyword.artist;
    auto content = co_await http.get();
    auto infos = parseInfoData(content);
    if (infos.size() > kMaxDownloadSize) {
        infos.resize(kMaxDownloadSize);
    }

    auto results = co_await downloadKLrc(infos);
    for (auto& result : results) {
        result.request_title = title;
        result.request_artist = artist;
    }
    emit fetchLyricsCompleted(results);
    co_return;
}

QCoro::Task<> BackgroundService::searchNetease(const PlayListEntity& keyword) {
    const auto search_text = (keyword.title + " "_str + keyword.artist).trimmed();
    http::HttpClient http(&nam_, "http://music.163.com/api/search/get"_str, this);
    http.param("s"_str, search_text.isEmpty() ? keyword.title : search_text);
    http.param("limit"_str, 20);
    http.param("offset"_str, 0);
    http.param("type"_str, 1);
    auto title = keyword.title;
    auto artist = keyword.artist;
    auto content = co_await http.get();
    std::optional<QList<NeteaseSong>> songs = parseNeteaseSong(content);
    if (!songs.has_value()) {
        co_return;
    }
    auto results = co_await downloadNeteaseLrc(*songs);
    for (auto &result : results) {
        result.request_title = title;
        result.request_artist = artist;
    }
    emit fetchLyricsCompleted(results);
	co_return;
}

QCoro::Task<> BackgroundService::searchLyrics(const PlayListEntity& keyword) {
    auto temp = keyword.cleanup();
    searchNetease(temp).then([this, temp]() {
        XAMP_LOG_DEBUG("Search Netease lyrics completed!");
        searchKugou(temp).then([]() {
            XAMP_LOG_DEBUG("Search Kugou lyrics completed!");
            });
        });
    co_return;
}

void BackgroundService::onSearchLyrics(const PlayListEntity& keyword) {
    auto temp = keyword.cleanup();
    searchLyrics(temp).then([]() {});
}

#if defined(Q_OS_WIN)
void BackgroundService::onFetchCdInfo(const DriveInfo& drive) {
    MBDiscId mbdisc_id;
    std::string disc_id;
    std::string url;

    try {
        disc_id = mbdisc_id.GetDiscId(drive.drive_path.toStdString());
        url = mbdisc_id.GetDiscIdLookupUrl(drive.drive_path.toStdString());
    } catch (const Exception &e) {
        XAMP_LOG_DEBUG(e.GetErrorMessage());
        return;
    }

    XAMP_LOG_D(logger_, "Start fetch cd information form musicbrainz.");

    http_client_.setUrl(QString::fromStdString(url));
    http_client_.get().then([this, drive, disc_id](const auto& content) {
        auto [image_url, mb_disc_id_info] = parseMbDiscIdXml(content);

        std::forward_list<TrackInfo> track_infos;
        const auto cd = OpenCD(drive.driver_letter);
        cd->SetMaxSpeed();
        const auto tracks = cd->GetTotalTracks();

        auto track_id = 0;
        for (const auto& track : tracks) {
            TrackInfo track_info;
            auto reader = MakeMetadataReader();
            reader->Open(track);
            auto track_info_opt = reader->Extract();
            if (track_info_opt.has_value()) {
				track_info = track_info_opt.value();
            }
            track_info.title = mb_disc_id_info.tracks[track_id].title;
            track_info.file_path = track;
            track_info.duration = cd->GetDuration(track_id++);
            track_info.album = mb_disc_id_info.album;
            track_info.sample_rate = AudioFormat::k16BitPCM441Khz.GetSampleRate();
            track_info.disc_id = disc_id;
            track_info.track = track_id;
            track_infos.push_front(track_info);
        }

        track_infos.sort([](const auto& first, const auto& last) {
            return first.track < last.track;
            });

        emit readCdTrackInfo(QString::fromStdString(disc_id), track_infos);

        mb_disc_id_info.disc_id = disc_id;
        std::sort(mb_disc_id_info.tracks.begin(), mb_disc_id_info.tracks.end(), 
            [](const auto& first, const auto& last) {
            return first.track < last.track;
            });

        emit fetchMbDiscInfoCompleted(mb_disc_id_info);

        XAMP_LOG_D(logger_, "Start fetch cd cover image.");

        http_client_.setUrl(QString::fromStdString(image_url));
        http_client_.get().then([this, disc_id](const auto& content) {
            const auto cover_url = parseCoverUrl(content);
            http_client_.setUrl(cover_url);
            http_client_.download().then([this, disc_id](const auto& content) mutable {
                QPixmap cover;
                if (cover.loadFromData(content)) {
                    const auto cover_id = qImageCache.addImage(cover);
                    XAMP_LOG_D(logger_, "Download cover image completed.");
                    emit fetchDiscCoverCompleted(QString::fromStdString(disc_id), cover_id);
                }
            });
        });
    });
}
#endif

void BackgroundService::onBlurImage(const QString& cover_id,
    const QPixmap& image, 
    QSize size) {
    if (image.isNull()) {
        emit blurImage(QImage());
        return;
    }    
    emit blurImage(blur_image_cache_.GetOrAdd(cover_id, [&]() {
        return image_util::blurImage(thread_pool_, image, size);
        }));
}

void BackgroundService::onTranslation(const QString& keyword, 
    const QString& from,
    const QString& to) {
    const auto url =
        qFormat("https://translate.google.com/translate_a/single?client=gtx&sl=%3&tl=%2&dt=t&q=%1")
        .arg(QString::fromStdString(QUrl::toPercentEncoding(keyword).toStdString()))
        .arg(to)
        .arg(from);
    http_client_.setUrl(url);
    http_client_.get().then([keyword, this](const auto& content) {
        if (content.isEmpty()) {
            return;
        }
        auto result = content;
        result = result.replace("[[[\""_str, ""_str);
        result = result.mid(0, result.indexOf(",\""_str) - 1);
        emit translationCompleted(keyword, result);
        });
}

void BackgroundService::onReadSpectrogram(SpectrogramColor color, const PlayListEntity& entity) {
	try {        
        ArchiveFileStream afs;

        if (entity.is_zip_file && entity.archive_entry_name.has_value()) {
            auto result = StreamFactory::MakeArchiveFileStream(
                entity.file_path.toStdWString(),
                entity.archive_entry_name.value().toStdWString());
            if (!result.has_value()) {
				throw Exception(result.error());
            }
            afs = std::move(result.value());
        }
        else {
            afs.file_stream = makePcmFileStream(entity.file_path.toStdWString());
        }
        if (afs.file_stream->GetDuration() <= 0.0) {
            return;
        }

        STFT fft(kFFTSize, kHopSize);
        fft.SetWindowType(WindowType::HANN);

        Buffer<float> buffer(kFFTSize);        
		
        int time_index = 0;
        auto duration = afs.file_stream->GetDuration();
        ComplexValarray freq_bins;
        freq_bins.resize(fft.GetShiftSize() + 1);

        auto format = afs.file_stream->GetFormat();
		auto total_samples = static_cast<uint32_t>(duration * format.GetSampleRate());
        auto kColumnsPerChunk = (std::min)(100u, static_cast<uint32_t>(total_samples / fft.GetShiftSize()));

        color_table_.setSpectrogramColor(color);

        while (!is_stop_ && afs.file_stream->IsActive()) {
        	QImage chunk_img(kColumnsPerChunk, 
                freq_bins.size(),
                QImage::Format_RGB888);
            chunk_img.fill(Qt::black);

            if (!getSamples(afs.file_stream, buffer)) {
                freq_bins = fft.Flush();
                fillSpectrogramColumn(color_table_, chunk_img, 0, freq_bins);
                emit readAudioSpectrogram(duration, kHopSize, chunk_img, time_index);
                break;
            }

            freq_bins = fft.Process(buffer.data(), buffer.size());
            fillSpectrogramColumn(color_table_, chunk_img, 0, freq_bins);

            int actual_columns = kColumnsPerChunk;
			
            for (int col = 1; col < kColumnsPerChunk; col++) {                             
				if (!getSamples(afs.file_stream, buffer)) {
                    actual_columns = col;
                    freq_bins = fft.Flush();
                    fillSpectrogramColumn(color_table_, chunk_img, col, freq_bins);
                    break;
                }
                else {
                    freq_bins = fft.Process(buffer.data(), buffer.size());
                    fillSpectrogramColumn(color_table_, chunk_img, col, freq_bins);
                }
            }
            emit readAudioSpectrogram(duration, kHopSize, chunk_img, time_index);
            time_index += actual_columns;
        }

        emit readAudioDataCompleted();
    }
    catch (const Exception& e) {
        XAMP_LOG_ERROR(e.GetErrorMessage());
    }
    catch (const std::exception& e) {
        XAMP_LOG_ERROR(e.what());
    }
}
