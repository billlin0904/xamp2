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
#include <widget/tagio.h>
#include <widget/krcparser.h>
#include <widget/lrcparser.h>
#include <widget/neteaseparser.h>
#include <widget/util/json_util.h>
#include <widget/musicbrainzparser.h>

#include <widget/util/read_util.h>
#include <widget/util/colortable.h>

#include <stream/filestream.h>
#include <stream/bassfilestream.h>
#include <stream/fft.h>
#include <stream/stft.h>
#include <stream/ebur128scanner.h>

#include <base/logger_impl.h>
#include <base/math.h>

#if defined(Q_OS_WIN)
#include <stream/mbdiscid.h>
#endif

#include <QDir>
#include <QFuture>
#include <QJsonValueRef>

namespace {
    XAMP_DECLARE_LOG_NAME(BackgroundService);

    constexpr size_t kFFTSize = 2048 * 2;
    constexpr size_t kHopSize = kFFTSize * 0.5;
    constexpr float kPower2FFSize = kFFTSize * kFFTSize;
    constexpr size_t kFreqBins = (kFFTSize / 2);    
   
    float getDb(const std::complex<float>& c) {
        // 取得 (實部^2 + 虛部^2)
        float val = (c.real() * c.real() + c.imag() * c.imag());
        if (val <= 0.0f) {
            return ColorTable::kMinDb;
        }
        // log10( val / kPower2FFSize ) → dB
        //float dB = 10.0f * std::log10f(val / kPower2FFSize);
        float dB = 10.0f * log10f_fast(val / kPower2FFSize);
        return dB;
    }

    float getRealDb(float real_val) {
        if (real_val == 0.0f) {
            return ColorTable::kMinDb;
        }
        //float dB = 10.0f * std::log10f((real_val * real_val) / kPower2FFSize);
        float dB = 10.0f * log10f_fast((real_val * real_val) / kPower2FFSize);
        return dB;
    }

    void fillSpectrogramColumn(const ColorTable &color_table,
        QImage& chunk_img,
        int col,
        const ComplexValarray& freq_bins) {
#if 0
        // 1) 先準備顏色查表物件 (靜態單例)
        // 2) 取得 QImage 的基本資訊
        uchar* data = chunk_img.bits();
        int bytes_per_line = chunk_img.bytesPerLine();
        int height = chunk_img.height();
        int n_bins = static_cast<int>(freq_bins.size());

        // 3) 計算線性縮放比例： freq_bins.size() → 影像高度
        //    預計 f=0 畫在最底部 (height-1)，f=n_bins-1 畫在最頂 (0)
        double scale = 1.0;
        if (n_bins > 1 && height > 1) {
            scale = static_cast<double>(height - 1) / static_cast<double>(n_bins - 1);
        }

        // 4) 逐一頻率 bin，計算 dB，並繪製到對應的 y
        for (int f = 0; f < n_bins; ++f) {
            float dB;
            if (f == 0 || f == n_bins - 1) {
                // 端點 bin (DC / Nyquist) 使用實數 dB
                dB = getRealDb(freq_bins[f].real());
            }
            else {
                // 其他 bin 使用複數幅度 dB
                dB = getDb(freq_bins[f]);
            }

            // 透過 color_table 查出對應的 QRgb (SoX 調色)
            QRgb color = color_table[dB];

            // 將 bin 索引 f => y 座標
            //   y = (height - 1) - round(f * scale)
            int scaled_f = static_cast<int>(f * scale + 0.5);
            int y = (height - 1) - scaled_f;

            // x = col 代表「這一整列」是第幾個時間窗
            int x = col;

            // 計算此像素在 QImage data 陣列的起始 index
            // QImage::Format_RGB888 => 每像素 3 bytes (R, G, B)
            int idx = y * bytes_per_line + x * 3;

            // 寫入 RGB
            data[idx + 0] = qRed(color);
            data[idx + 1] = qGreen(color);
            data[idx + 2] = qBlue(color);
        }
#else
        // --- 前置 -----------------------------------------------------------------
        const int h = chunk_img.height();
        const int stride = chunk_img.bytesPerLine();
        const int n = static_cast<int>(freq_bins.size());

        const int num = h - 1;
        const int den = n - 1;

        uchar* base = chunk_img.bits() + col * 3;           // 提前算 x 位移
        auto  toRow = [&](int f) noexcept {
            return ((h - 1) - (f * num + den / 2) / den) * stride;
            };

        // --- 主迴圈 ----------------------------------------------------------------
        for (int f = 0; f < n; ++f) {
            // 1. 轉 dB（示範：直接 norm→log10，不採 LUT）
            float db;
            if ((f == 0) || (f == n - 1)) {              // 無分支：bit-or
                db = getRealDb(freq_bins[f].real());
            }
            else {
                db = getDb(freq_bins[f]);
            }

            // 2. 顏色查表 (LUT 推薦)
            const QRgb c = color_table[db];

            // 3. 寫像素
            uchar* pix = base + toRow(f);
            pix[0] = qRed(c);
            pix[1] = qGreen(c);
            pix[2] = qBlue(c);
        }
#endif
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
    logger_ = XampLoggerFactory.GetLogger(XAMP_LOG_NAME(BackgroundService));
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

    auto file_name = getValidFileName(job.file.file_name);

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

    Path input_path(job.file.file_path.toStdWString());
    auto stop_token = stop_source_.get_token();

    try {
        auto encoder = StreamFactory::MakeFileEncoder();

        AnyMap config;
        config.AddOrReplace(FileEncoderConfig::kInputFilePath,
            input_path);
        config.AddOrReplace(FileEncoderConfig::kOutputFilePath,
            output_path);
        config.AddOrReplace(FileEncoderConfig::kCodecId,
            job.codec_id.toStdString());
        config.AddOrReplace(FileEncoderConfig::kBitRate,
            job.bit_rate);

        encoder->Start(config, file_writer);
        encoder->Encode([job, &stop_token, this](auto progress) {
            if (stop_token.stop_requested()) {
                return false;
            }
            if (progress % 10 == 0) {
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

        TagIO output_io;
        output_io.Open(output_path, TAG_IO_WRITE_MODE);
        output_io.writeArtist(job.file.artist);
        output_io.writeTitle(job.file.title);
        output_io.writeAlbum(job.file.album);
        output_io.writeComment(job.file.comment);
        output_io.writeGenre(job.file.genre);
        output_io.writeTrack(job.file.track);
        output_io.writeYear(job.file.year);

        TagIO input_io;
        input_io.Open(input_path, TAG_IO_READ_MODE);
        auto cover = input_io.embeddedCover();
        if (!cover.isNull()) {
            output_io.writeEmbeddedCover(cover);
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

QCoro::Task<> BackgroundService::fetchMusicBrainzRecording(const PlayListEntity& entity) {
    PlayListEntity temp = entity;
    http_client_.setUrl("https://api.acoustid.org/v2/lookup"_str);
    http_client_.param("client"_str, "J0OsCydP14"_str);
    http_client_.param("format"_str, "json"_str);
    http_client_.param("meta"_str, "recordings+releasegroups+releases+tracks+compress"_str);
    http_client_.param("duration"_str, static_cast<uint32_t>(Round(entity.duration)));
    http_client_.param("fingerprint"_str, readChromaprint(entity.file_path.toStdWString()));
    auto content = co_await http_client_.get();
    auto resp = acoustid::parseAcoustidResponse(content);
    std::optional<acoustid::Recording> search_recording;
    if (!resp) {
        co_return;
    }
    for (const auto& result : resp.value().results) {
        for (const auto& recording : result.recordings) {
            if (temp.title.contains(recording.title)) {
                search_recording = recording;
                break;
            }
        }
    }
    if (!search_recording) {
        co_return;
    }
    http_client_.setUrl("https://musicbrainz.org/ws/2/recording/"_str + search_recording.value().id);
    http_client_.param("inc"_str, "artist-credits+tags+genres"_str);
    http_client_.param("fmt"_str, "json"_str);
    content = co_await http_client_.get();
    auto root_recording = musicbrain::parseRootRecording(content);
    if (!root_recording) {
        co_return;
    }
    QList<QString> genres;
    for (const auto& tag : root_recording.value().genres) {
        genres.append(tag.name);
    }
    co_return;
}

void BackgroundService::parallelEncode(const QString& dir_name, QList<EncodeJob> jobs) {
    auto stop_token = stop_source_.get_token();
    Executor::ParallelFor(thread_pool_, jobs,
        [this, dir_name](const EncodeJob& job) {
        executeEncodeJob(dir_name, job);
        }, stop_token);
}

void BackgroundService::onAddJobs(const QString& dir_name, const QList<EncodeJob>& jobs) {
    stop_source_ = std::stop_source();

    QList<EncodeJob> parallel_jobs;
	Q_FOREACH(auto job, jobs) {
		if (job.file.disc_id.isEmpty()) {
			parallel_jobs.push_back(job);
		}
		else {
			sequenceEncode(dir_name, { job });
		}
    }
    parallelEncode(dir_name, parallel_jobs);
}

void BackgroundService::cancelRequested() {
    is_stop_ = true;
}

QCoro::Task<SearchLyricsResult> BackgroundService::downloadSingleNeteaseLrc(NeteaseSong info) {
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

    http_client_.setUrl("http://mobilecdn.kugou.com/api/v3/search/song"_str);
    http_client_.param("format"_str, "json"_str);
    http_client_.param("keyword"_str, keyword.title);

    auto content = co_await http_client_.get();
    auto infos = parseInfoData(content);
    infos.resize(kMaxDownloadSize);

    auto results = co_await downloadKLrc(infos);
    emit fetchLyricsCompleted(results);
    co_return;
}

QCoro::Task<> BackgroundService::searchNetease(const PlayListEntity& keyword) {
    http_client_.setUrl("http://music.163.com/api/search/get"_str);
    http_client_.param("s"_str, keyword.title);
    http_client_.param("limit"_str, 20);
    http_client_.param("offset"_str, 0);
    http_client_.param("type"_str, 1);

    auto content = co_await http_client_.get();
    std::optional<QList<NeteaseSong>> songs = parseNeteaseSong(content);
    if (!songs.has_value()) {
        co_return;
    }
    auto results = co_await downloadNeteaseLrc(*songs);
    emit fetchLyricsCompleted(results);
	co_return;
}

void BackgroundService::onSearchLyrics(const PlayListEntity& keyword) {
    auto temp = keyword.cleanup();
    searchKugou(temp).then([]() {
        XAMP_LOG_DEBUG("Search Kugou lyrics completed!");
        });
    searchNetease(temp).then([]() {
        XAMP_LOG_DEBUG("Search Netease lyrics completed!");
        });
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
            auto track_info = TagIO::getTrackInfo(track);
            if (track_info) {
                track_info.value().file_path = tracks[track_id];
                track_info.value().duration = cd->GetDuration(track_id++);
                track_info.value().album = mb_disc_id_info.album;
                track_info.value().sample_rate = 44100;
                track_info.value().disc_id = disc_id;
                track_info.value().track = track_id;
                track_infos.push_front(track_info.value());
            }
            else {
                XAMP_LOG_D(logger_, "Failed to extract track info.");
                return;
            }
        }

        track_infos.sort([](const auto& first, const auto& last) {
            return first.track < last.track;
            });

        emit readCdTrackInfo(QString::fromStdString(disc_id), track_infos);

        mb_disc_id_info.disc_id = disc_id;
        mb_disc_id_info.tracks.sort([](const auto& first, const auto& last) {
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

void BackgroundService::onReadSpectrogram(SpectrogramColor color, const Path& file_path) {
	try {
        auto file_stream = makePcmFileStream(file_path);
        if (file_stream->GetDurationAsSeconds() <= 0.0) {
            return;
        }

        STFT fft(kFFTSize, kHopSize);
        fft.SetWindowType(WindowType::HANN);

        Buffer<float> buffer(kFFTSize);
		
        int time_index = 0;
        auto duration = file_stream->GetDurationAsSeconds();
        ComplexValarray freq_bins;
        freq_bins.resize(fft.GetShiftSize() + 1);

        auto format = file_stream->GetFormat();
		auto total_samples = static_cast<uint32_t>(duration * format.GetSampleRate());
        auto kColumnsPerChunk = (std::min)(100u, static_cast<uint32_t>(total_samples / fft.GetShiftSize()));

        color_table_.setSpectrogramColor(color);

        while (!is_stop_ && file_stream->IsActive()) {
        	QImage chunk_img(kColumnsPerChunk, 
                freq_bins.size(),
                QImage::Format_RGB888);
            chunk_img.fill(Qt::black);

            if (!getSamples(file_stream, buffer)) {
                freq_bins = fft.Flush();
                fillSpectrogramColumn(color_table_, chunk_img, 0, freq_bins);
                emit readAudioSpectrogram(duration, kHopSize, chunk_img, time_index);
                break;
            }

            freq_bins = fft.Process(buffer.data(), buffer.size());
            fillSpectrogramColumn(color_table_, chunk_img, 0, freq_bins);

            int actual_columns = kColumnsPerChunk;
			
            for (int col = 1; col < kColumnsPerChunk; col++) {                             
				if (!getSamples(file_stream, buffer)) {
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
