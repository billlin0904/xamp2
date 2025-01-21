#include <widget/worker/backgroundservice.h>

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

#include <stream/filestream.h>
#include <stream/fft.h>
#include <stream/stft.h>
#include <base/logger_impl.h>

#if defined(Q_OS_WIN)
#include <stream/mbdiscid.h>
#endif

#include <QDir>
#include <QJsonValueRef>
#include <QThread>

XAMP_DECLARE_LOG_NAME(BackgroundService);

namespace {
    QString uniqueFileName(const QDir& dir, const QString& originalName) {
        QFileInfo info(originalName);
        QString baseName = info.completeBaseName();
        QString extension = info.suffix().isEmpty() ? ""_str : "."_str + info.suffix();

        QString candidate = originalName;
        int counter = 1;
        
        while (dir.exists(candidate)) {
            candidate = qFormat("%1(%2)%3").arg(baseName).arg(counter).arg(extension);
            counter++;
        }
        return candidate;
    }

    QString getValidFileName(QString fileName) {
        static const QRegularExpression forbidden_pattern(R"([\*\?\"<>:/\\\|])"_str);
        fileName.replace(forbidden_pattern, " "_str);
		return fileName;
    }

    constexpr size_t kFFTSize = 4096;
    constexpr size_t kHopSize = kFFTSize * 0.25;
    constexpr float kPower2FFSize = kFFTSize * kFFTSize;
    constexpr size_t kFreqBins = (kFFTSize / 2);

    constexpr double kMaxDb = 0.0;
    constexpr double kMinDb = -120.0;
    constexpr double kDbRange = kMaxDb - kMinDb;

    class ColorTable {
    public:
        static constexpr int kTableSize = kFFTSize / 2;

        ColorTable() {
            for (int i = 0; i < kTableSize; ++i) {
                double ratio = static_cast<double>(i)
            	/ static_cast<double>(kTableSize - 1);
                lut_[i] = makeSoxrColor(ratio);
            }
        }

		QRgb operator[](double dB_val) const {
            if (dB_val < kMinDb)
                dB_val = kMinDb;

            if (dB_val > kMaxDb)
                dB_val = kMaxDb;

            double ratio = (dB_val + kDbRange) / kDbRange;
            int idx = static_cast<int>(ratio * (kTableSize - 1) + 0.5);

            if (idx < 0)
                idx = 0;
            else if (idx >= kTableSize) 
                idx = kTableSize - 1;

			return lut_[idx];
		}

        static QRgb makeColor(double level) {
            level *= 0.6625;
            double r = 0.0, g = 0.0, b = 0.0;
            if (level >= 0 && level < 0.15) {
                r = (0.15 - level) / (0.15 + 0.075);
                g = 0.0;
                b = 1.0;
            }
            else if (level >= 0.15 && level < 0.275) {
                r = 0.0;
                g = (level - 0.15) / (0.275 - 0.15);
                b = 1.0;
            }
            else if (level >= 0.275 && level < 0.325) {
                r = 0.0;
                g = 1.0;
                b = (0.325 - level) / (0.325 - 0.275);
            }
            else if (level >= 0.325 && level < 0.5) {
                r = (level - 0.325) / (0.5 - 0.325);
                g = 1.0;
                b = 0.0;
            }
            else if (level >= 0.5 && level < 0.6625) {
                r = 1.0;
                g = (0.6625 - level) / (0.6625 - 0.5f);
                b = 0.0;
            }

            // Intensity correction.
            double cf = 1.0;
            if (level >= 0.0 && level < 0.1) {
                cf = level / 0.1;
            }
            cf *= 255.0;

            // Pack RGB values into a 32-bit uint.
            uint32_t rr = static_cast<uint32_t>(r * cf + 0.5);
            uint32_t gg = static_cast<uint32_t>(g * cf + 0.5);
            uint32_t bb = static_cast<uint32_t>(b * cf + 0.5);

            return qRgb(rr, gg, bb);
        }

        static QRgb makeSoxrColor(double level) {
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
    private:
        std::array<QRgb, kTableSize> lut_;
    };

    auto getDb(const std::complex<float>& complex) -> float {
        float val = (complex.real() * complex.real() 
            + complex.imag() * complex.imag());
        if (val != 0.0f) {
            float dB = 10.0f * log10f(val / kPower2FFSize);
            return dB;
        }
        return -120.f;
    }

    auto getRealDb(const float real) -> float {
        if (real != 0.0f) {
            float dB = 10.0f * log10f((real * real) / kPower2FFSize);
            return dB;
        }
        return -120.f;
    }

    auto makePcmFileStream(const Path& file_path) -> ScopedPtr<FileStream> {
        auto dsd_mode = DsdModes::DSD_MODE_DSD2PCM;
        if (!IsDsdFile(file_path)) {
            dsd_mode = DsdModes::DSD_MODE_PCM;
        }
        auto file_stream =
            StreamFactory::MakeFileStream(file_path, dsd_mode);
        file_stream->OpenFile(file_path);
        return file_stream;
    }
}

BackgroundService::BackgroundService()
    : nam_(this)
	, http_client_(&nam_, QString(), this)
    , buffer_pool_(MakeObjectPool<QByteArray>(kBufferPoolSize)) {
    logger_ = XampLoggerFactory.GetLogger(XAMP_LOG_NAME(BackgroundService));
    thread_pool_ = ThreadPoolBuilder::MakeBackgroundThreadPool();
}

BackgroundService::~BackgroundService() = default;

void BackgroundService::cancelAllJob() {
    stop_source_.request_stop();
}

void BackgroundService::onAddJobs(const QString& dir_name,
    QList<EncodeJob> jobs) {
    stop_source_ = std::stop_source();
    
    Executor::ParallelFor(thread_pool_.get(),
        stop_source_.get_token(),
        jobs, [dir_name, this](auto &job, const auto &stop_token) {
        std::shared_ptr<IFile> file_writer;
        Path output_path;
        
        auto file_name = getValidFileName(job.file.title);

		// Ensure file name is unique.
        constexpr auto kMaxRetryTestUniqueFileName = 255;
        auto i = 0;
        for (; i < kMaxRetryTestUniqueFileName; ++i) {
			try {
                auto unique_save_file_name = uniqueFileName(
                    QDir(dir_name),
                    file_name + ".m4a"_str);
                //auto unique_save_file_name = uniqueFileName(QDir(dir_name)
                //, job.file.title + ".wav"_str);
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
            //config.AddOrReplace(FileEncoderConfig::kCodecId,
            //std::string("pcm"));
            //config.AddOrReplace(FileEncoderConfig::kBitRate,
            //static_cast<uint32_t>(0));

            encoder->Start(config, file_writer);
            encoder->Encode(stop_token, [job, this](auto progress) {
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
        catch (const std::exception& e) {
            XAMP_LOG_ERROR(e.what());
            emit jobError(job.job_id, tr("Error"));
        }
        });
}

void BackgroundService::cancelRequested() {
    is_stop_ = true;
}

void BackgroundService::onSearchLyrics(int32_t music_id,
    const QString& title, 
    const QString& artist) {
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

    try {
        ForwardList<TrackInfo> track_infos;
        const auto cd = OpenCD(drive.driver_letter);
        cd->SetMaxSpeed();
        const auto tracks = cd->GetTotalTracks();

        auto track_id = 0;
        for (const auto& track : tracks) {
            auto track_info = TagIO::getTrackInfo(track);
            track_info.file_path = tracks[track_id];
            track_info.duration = cd->GetDuration(track_id++);
            track_info.sample_rate = 44100;
            track_info.disc_id = disc_id;
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

    XAMP_LOG_D(logger_, "Start fetch cd information form music brainz.");

    http_client_.setUrl(QString::fromStdString(url));
    http_client_.get().then([this, disc_id](const auto& content) {
        auto [image_url, mb_disc_id_info] = parseMbDiscIdXml(content);

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
            if (read_samples == 0) {
                break;
            }
            emit readAudioData(buffer);
        }

        emit readAudioDataCompleted();
    }
	catch (const Exception& e) {
		XAMP_LOG_ERROR(e.GetErrorMessage());
	}
}

void BackgroundService::onReadSpectrogram(const QSize& widget_size, const Path& file_path) {
    static const ColorTable color_lut;

    try {
		auto file_stream = makePcmFileStream(file_path);

    	STFT fft(kFFTSize, kHopSize);
        fft.SetWindowType(WindowType::HANN);

        size_t time_index = 0;
        std::vector<float> buffer(kFFTSize);

        const double duration_sec = file_stream->GetDurationAsSeconds();
        size_t max_time_bins = static_cast<size_t>(std::ceil(
            duration_sec 
            * file_stream->GetFormat().GetSampleRate() / kHopSize)) + 1;

        QSize image_size(max_time_bins, kFreqBins + 1);

        // Resize big image for spectrogram
        QImage spec_img(image_size, QImage::Format_RGB888);
        spec_img.fill(Qt::black);

        uchar* data = spec_img.bits();
        int bytes_per_line = spec_img.bytesPerLine();

		auto spec_img_width = spec_img.width();
        auto spec_img_height = spec_img.height();

        // Read audio data and calculate spectrogram
        while (!is_stop_ && file_stream->IsActive()) {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            auto read_samples = file_stream->GetSamples(
                buffer.data(), 
                buffer.size());
            if (read_samples == 0) {
                break;
            }
            if (read_samples < buffer.size() / 2) {
                XAMP_LOG_WARN("read_samples small than buffer size.");
            }

            const auto& freq_bins = fft.Process(
                buffer.data(), 
                buffer.size());

            for (size_t f = 0; f < freq_bins.size(); f++) {
                double dB = 0.0;
                if (f == 0 || f == freq_bins.size() - 1) {
                    dB = getRealDb(freq_bins[f].real());
                }
                else {
                    dB = getDb(freq_bins[f]);
                }

            	//dB = -110;
                QRgb color = color_lut[dB];
                const int y = static_cast<int>(
                    freq_bins.size()) - 1 - static_cast<int>(f);
                const int x = static_cast<int>(time_index);

                if (x >= 0 && x < spec_img_width &&
					y >= 0 && y < spec_img_height) {
                    int idx = y * bytes_per_line + x * 3;
                    uchar r = qRed(color);
                    uchar g = qGreen(color);
                    uchar b = qBlue(color);

                    data[idx + 0] = r;
                    data[idx + 1] = g;
                    data[idx + 2] = b;
                }
            }
            time_index++;
        }

        // Resize image to fit the spectrogram
        auto final_img = spec_img.copy(0,
            0,
            static_cast<int>(time_index),
            static_cast<int>(kFreqBins + 1)
        );
        emit readAudioSpectrogram(final_img);
	}
	catch (const Exception& e) {
		XAMP_LOG_ERROR(e.GetErrorMessage());
    }
	catch (const std::exception& e) {
		XAMP_LOG_ERROR(e.what());
	}
}
