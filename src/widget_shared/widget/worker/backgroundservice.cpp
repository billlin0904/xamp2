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

#include <stream/filestream.h>
#include <stream/fft.h>
#include <stream/stft.h>
#include <base/logger_impl.h>

#if defined(Q_OS_WIN)
#include <stream/mbdiscid.h>
#endif

#include <QDir>
#include <QFuture>
#include <QJsonValueRef>

XAMP_DECLARE_LOG_NAME(BackgroundService);

namespace {
    constexpr size_t kFFTSize = 4096 * 2;
    constexpr size_t kHopSize = kFFTSize * 0.5;
    constexpr float kPower2FFSize = kFFTSize * kFFTSize;
    constexpr size_t kFreqBins = (kFFTSize / 2);

    constexpr double kMaxDb = 0;
    constexpr double kMinDb = -120.0;
    constexpr double kDbRange = kMaxDb - kMinDb;

    class ColorTable {
    public:
        ColorTable() = default;

        QRgb operator[](double dB_val) const {
            if (dB_val < kMinDb) dB_val = kMinDb;
            if (dB_val > kMaxDb) dB_val = kMaxDb;
            const double ratio = (dB_val - kMinDb) / kDbRange;
            return soxrColor(ratio);
        }

    private:
        static QRgb soxrColor(double level) {
            double r = 0.0;
            if (level >= 0.13 && level < 0.73) {
                r = sin((level - 0.13) / 0.60 * XAMP_PI / 2.0);
            }
            else if (level >= 0.73) {
                r = 1.0;
            }

            double g = 0.0;
            if (level >= 0.6 && level < 0.91) {
                g = sin((level - 0.6) / 0.31 * XAMP_PI / 2.0);
            }
            else if (level >= 0.91) {
                g = 1.0;
            }

            double b = 0.0;
            if (level < 0.60) {
                b = 0.5 * sin(level / 0.6 * XAMP_PI);
            }
            else if (level >= 0.78) {
                b = (level - 0.78) / 0.22;
            }

            // clamp b
            if (b < 0.) b = 0.;
            if (b > 1.) b = 1.;

            uint32_t rr = static_cast<uint32_t>(r * 255.0 + 0.5);
            uint32_t gg = static_cast<uint32_t>(g * 255.0 + 0.5);
            uint32_t bb = static_cast<uint32_t>(b * 255.0 + 0.5);

            return qRgb(rr, gg, bb);
        }
    };

    float getDb(const std::complex<float>& c) {
        // 取得 (實部^2 + 虛部^2)
        float val = (c.real() * c.real() + c.imag() * c.imag());
        if (val <= 0.0f) {
            return kMinDb;
        }
        // log10( val / kPower2FFSize ) → dB
        float dB = 10.0f * std::log10f(val / kPower2FFSize);
        return dB;
    }

    float getRealDb(float real_val) {
        if (real_val == 0.0f) {
            return kMinDb;
        }
        float dB = 10.0f * std::log10f((real_val * real_val) / kPower2FFSize);
        return dB;
    }

    void fillSpectrogramColumn(
        QImage& chunk_img,
        int col,
        const ComplexValarray& freq_bins
    ) {
        // 1) 先準備顏色查表物件 (靜態單例)
        static ColorTable color_table;

        // 2) 取得 QImage 的基本資訊
        uchar* data = chunk_img.bits();
        int bytes_per_line = chunk_img.bytesPerLine();
        int height = chunk_img.height();
        int n_bins = static_cast<int>(freq_bins.size());

        // 3) 計算線性縮放比例： freq_bins.size() → 影像高度
        //    預計 f=0 畫在最底部 (height-1)，f=n_bins-1 畫在最頂 (0)
        double scale = 1.0;
        if (n_bins > 1 && height > 1) {
            scale = double(height - 1) / double(n_bins - 1);
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

void BackgroundService::onAddJobs(const QString& dir_name,
    const QList<EncodeJob>& jobs) {
    stop_source_ = std::stop_source();

    Q_FOREACH(auto job, jobs) {
        std::shared_ptr<IFile> file_writer;
        Path output_path;

        auto file_name = getValidFileName(job.file.title);

        // Ensure file name is unique.
        constexpr auto kMaxRetryTestUniqueFileName = 255;
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
                        QDir(dir_name)
                        , job.file.title + ".wav"_str);
                    break;
                }

                auto save_file_name = dir_name
                    + "/"_str
                    + unique_save_file_name;
                output_path = save_file_name.toStdWString();
                file_writer = MakFileEncodeWriter(output_path);
                break;
            }
            catch (const Exception& e) {
                XAMP_LOG_ERROR(e.GetErrorMessage());
                emit jobError(job.job_id, tr("Error"));
            }
        }
        if (i == kMaxRetryTestUniqueFileName) {
            XAMP_LOG_ERROR("Failed to create unique file name.");
            return;
        }

        Path input_path(job.file.file_path.toStdWString());

        try {

            auto encoder = StreamFactory::MakeM4AEncoder();

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
            encoder->Encode([job, this](auto progress) {
                if (progress % 10 == 0) {
                    emit updateJobProgress(job.job_id, progress);
                }
                return true;
                });
            file_writer.reset();
            encoder.reset();

            TagIO input_io;
            input_io.Open(input_path, TAG_IO_READ_MODE);

            TagIO output_io;
            output_io.Open(output_path, TAG_IO_WRITE_MODE);
            output_io.writeArtist(job.file.artist);
            output_io.writeTitle(job.file.title);
            output_io.writeAlbum(job.file.album);
            output_io.writeComment(job.file.comment);
            output_io.writeGenre(job.file.genre);
            output_io.writeTrack(job.file.track);
            output_io.writeYear(job.file.year);

            auto cover = input_io.embeddedCover();
            if (!cover.isNull()) {
                output_io.writeEmbeddedCover(cover);
            }

            emit updateJobProgress(job.job_id, 100);
        }
        catch (const Exception& e) {
            XAMP_LOG_ERROR(e.GetStackTrace());
            emit jobError(job.job_id, tr("Error"));
        }
        catch (const std::exception& e) {
            XAMP_LOG_ERROR(e.what());
            emit jobError(job.job_id, tr("Error"));
        }
    }
}

void BackgroundService::cancelRequested() {
    is_stop_ = true;
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

void BackgroundService::onSearchLyrics(const PlayListEntity& keyword) {
    http_client_.setUrl("http://mobilecdn.kugou.com/api/v3/search/song"_str);
    http_client_.param("format"_str, "json"_str);
    http_client_.param("keyword"_str, keyword.title);

    http_client_.get().then([this, keyword](const auto& content) {
        auto infos = parseInfoData(content);
        downloadKLrc(infos).then([this](const auto& results) {
			emit fetchLyricsCompleted(results);
        });
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

        try {
            std::forward_list<TrackInfo> track_infos;
            const auto cd = OpenCD(drive.driver_letter);
            cd->SetMaxSpeed();
            const auto tracks = cd->GetTotalTracks();

            auto track_id = 0;
            for (const auto& track : tracks) {
                auto track_info = TagIO::getTrackInfo(track);
                track_info.file_path = tracks[track_id];
                track_info.duration = cd->GetDuration(track_id++);
                track_info.album = mb_disc_id_info.album;
                track_info.sample_rate = 44100;
                track_info.disc_id = disc_id;
                track_info.track = track_id;
                track_infos.push_front(track_info);
            }

            track_infos.sort([](const auto& first, const auto& last) {
                return first.track < last.track;
                });

            emit readCdTrackInfo(QString::fromStdString(disc_id), track_infos);
        }
        catch (const Exception& e) {
            XAMP_LOG_DEBUG(e.GetErrorMessage());
            return;
        }

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

void BackgroundService::onReadWaveformAudioData(size_t frame_per_peek,
    const Path& file_path) {

    try {
        auto file_stream = makePcmFileStream(file_path);

        std::vector<float> buffer(
            frame_per_peek * file_stream->GetFormat().GetChannels());

        while (!is_stop_ && file_stream->IsActive()) {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            auto read_samples = file_stream->GetSamples(
                buffer.data(),
                buffer.size());
            if (read_samples > 0) {
                emit readAudioData(buffer);
            }
        }

        emit readAudioDataCompleted();
    }
	catch (const Exception& e) {
		XAMP_LOG_ERROR(e.GetErrorMessage());
	}
}

void BackgroundService::onReadSpectrogram(const QSize& widget_size, const Path& file_path) {
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

        int kColumnsPerChunk = (std::min)(100u, (file_stream->GetFormat().GetSampleRate() / 44100) * 100);

        while (!is_stop_ && file_stream->IsActive()) {        
        	QImage chunk_img(kColumnsPerChunk, freq_bins.size(), QImage::Format_RGB888);
            chunk_img.fill(Qt::black);

	        buffer.Fill(0.0f);
            auto read_samples = file_stream->GetSamples(buffer.data(),
                buffer.size());
            if (read_samples == 0) {
                freq_bins = fft.Flush();
                fillSpectrogramColumn(chunk_img, 0, freq_bins);
                emit readAudioSpectrogram(duration, chunk_img, time_index);
                break;
            }

            freq_bins = fft.Process(buffer.data(), buffer.size());
            fillSpectrogramColumn(chunk_img, 0, freq_bins);
            int actual_columns = kColumnsPerChunk;

            for (int col = 1; col < kColumnsPerChunk; col++) {
                buffer.Fill(0.0f);
                read_samples = file_stream->GetSamples(buffer.data(), 
                    buffer.size());
                if (read_samples == 0) {
                    actual_columns = col;
                    freq_bins = fft.Flush();
                    fillSpectrogramColumn(chunk_img, col, freq_bins);
                    break;
                }
                freq_bins = fft.Process(buffer.data(), buffer.size());
                fillSpectrogramColumn(chunk_img, col, freq_bins);
            }

            emit readAudioSpectrogram(duration, chunk_img, time_index);
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
