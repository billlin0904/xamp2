#include <QPainter>
#include <QLinearGradient>
#include <QTimer>
#include <QMouseEvent>
#include <QPointer>
#include <QThread>
#include <QVector>

#include <base/math.h>
#include <stream/bassfilestream.h>
#include <stream/fft.h>
#include <stream/filestream.h>
#include <widget/util/str_util.h>
#include <widget/util/ui_util.h>
#include <widget/util/image_util.h>
#include <widget/util/read_util.h>
#include <widget/chatgpt/spectrogramwidget.h>
#include <widget/actionmap.h>
#include <widget/appsettings.h>
#include <widget/appsettingnames.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <functional>
#include <thread>
#include <vector>

namespace {
    constexpr size_t kFFTSize = 2048 * 2;
    constexpr size_t kHopSize = kFFTSize * 0.5;
    constexpr float kPower2FFSize = kFFTSize * kFFTSize;

    auto makeImage(double duration_sec, uint32_t sample_rate, size_t hop_size, const QImage& chunk) -> QImage {
        size_t max_time_bins = static_cast<size_t>(std::ceil(
            duration_sec
            * sample_rate / hop_size)) + 1;

        QSize image_size(max_time_bins, chunk.height());
        QImage spec_img(image_size, QImage::Format_RGB888);
        spec_img.fill(Qt::black);
        return spec_img;
    }

    float toDbFromNorm(float p) {
        if (p <= 0.0f) {
            return ColorTable::kMinDb;
        }
        return 10.0f * log10f_fast(p / kPower2FFSize);
    }

    float getDb(const std::complex<float>& c) {
        return toDbFromNorm(std::norm(c));
    }

    float getRealDb(float r) {
        return toDbFromNorm(r * r);
    }

    uint32_t readFloatFrames(ScopedPtr<FileStream>& file_stream,
        Buffer<float>& buffer,
        uint16_t channels,
        uint32_t frames) {
        auto retry_count = 0;
        auto* bass_file_stream = dynamic_cast<BassFileStream*>(file_stream.get());
        if (!bass_file_stream || channels == 0 || frames == 0) {
            return 0;
        }

        const auto samples_to_read = frames * channels;
        constexpr auto kMaxRetryCount = 4;
        while (true) {
            buffer.Fill(0.0f);
            const auto samples_read = file_stream->GetSamples(buffer.data(), samples_to_read);
            if (samples_read > 0) {
                return samples_read / channels;
            }
            if (retry_count < kMaxRetryCount) {
                if (!bass_file_stream->EndOfStream()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    retry_count++;
                    continue;
                }
            }
            return 0;
        }
    }

    struct SpectrogramReadRequest {
        SpectrogramColor color{ SpectrogramColor::SPECTROGRAM_COLOR_DEFAULT };
        PlayListEntity entity;
        int target_columns{ 1 };
    };

    void shiftLeft(std::vector<float>& buffer, size_t count) {
        if (count >= buffer.size()) {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            return;
        }
        std::move(buffer.begin() + count, buffer.end(), buffer.begin());
        std::fill(buffer.end() - count, buffer.end(), 0.0f);
    }

    float downmixFrame(const float* samples, uint16_t channels) {
        if (channels == 0) {
            return 0.0f;
        }

        float mixed = 0.0f;
        for (uint16_t channel = 0; channel < channels; ++channel) {
            mixed += samples[channel];
        }
        return mixed / static_cast<float>(channels);
    }

    QImage makeSpectrogramChunk(const ColorTable& color_table,
        int begin_column,
        int end_column,
        int bands,
        const std::vector<double>& db_sums,
        const std::vector<uint32_t>& counts) {
        const auto columns = static_cast<int>(counts.size());
        const auto chunk_columns = (std::max)(0, end_column - begin_column);
        QImage image(chunk_columns, bands, QImage::Format_RGB888);
        image.fill(Qt::black);

        for (int x = begin_column; x < end_column; ++x) {
            auto source_x = x;
            if (counts[source_x] == 0) {
                auto left = x - 1;
                auto right = x + 1;
                while (left >= begin_column || right < end_column) {
                    if (left >= begin_column && counts[left] > 0) {
                        source_x = left;
                        break;
                    }
                    if (right < end_column && counts[right] > 0) {
                        source_x = right;
                        break;
                    }
                    --left;
                    ++right;
                }
            }

            const auto count = counts[source_x];
            if (count == 0) {
                continue;
            }

            for (int band = 0; band < bands; ++band) {
                const auto db = db_sums[source_x * bands + band] / static_cast<double>(count);
                const auto color = color_table[db];
                image.setPixel(x - begin_column, bands - band - 1, color);
            }
        }

        return image;
    }

    class SpectrogramWorker final : public QObject {
    public:
        using MetadataCallback = std::function<void(double, uint32_t, int, int)>;
        using ChunkCallback = std::function<void(int, const QImage&)>;
        using ErrorCallback = std::function<void(const QString&)>;
        using FinishedCallback = std::function<void()>;

        void cancel() {
            cancelled_.store(true, std::memory_order_relaxed);
        }

        void read(SpectrogramReadRequest request,
            MetadataCallback metadata_ready,
            ChunkCallback chunk_ready,
            ErrorCallback error_ready,
            FinishedCallback finished) {
            constexpr int kChunkColumns = 32;

            try {
                request.target_columns = (std::max)(1, request.target_columns);
                ArchiveFileStream afs;

                if (request.entity.is_zip_file && request.entity.archive_entry_name.has_value()) {
                    auto archive_result = StreamFactory::MakeArchiveFileStream(
                        request.entity.file_path.toStdWString(),
                        request.entity.archive_entry_name.value().toStdWString());
                    if (!archive_result.has_value()) {
                        throw Exception(archive_result.error());
                    }
                    afs = std::move(archive_result.value());
                }
                else {
                    afs.file_stream = makePcmFileStream(request.entity.file_path.toStdWString());
                }

                if (!afs.file_stream || afs.file_stream->GetDuration() <= 0.0) {
                    finished();
                    return;
                }

                const auto duration_sec = afs.file_stream->GetDuration();
                const auto format = afs.file_stream->GetFormat();
                const auto sample_rate = format.GetSampleRate();
                const auto channels = (std::max<uint16_t>)(1, format.GetChannels());
                const auto total_frames = (std::max<uint64_t>)(1,
                    static_cast<uint64_t>(std::llround(duration_sec * sample_rate)));

                Window window;
                window.Init(kFFTSize, WindowType::HANN);

                FFT fft;
                fft.Initialize(kFFTSize);

                const auto bands = static_cast<int>((kFFTSize / 2) + 1);
                metadata_ready(duration_sec, sample_rate, request.target_columns, bands);

                std::vector<float> fft_input(kFFTSize, 0.0f);
                Buffer<float> read_buffer(kHopSize * channels + 1024);
                std::vector<double> db_sums(request.target_columns * bands, 0.0);
                std::vector<uint32_t> counts(request.target_columns, 0);

                ColorTable color_table;
                color_table.setSpectrogramColor(request.color);

                int emitted_column = 0;
                uint64_t processed_frames = 0;
                while (!cancelled_.load(std::memory_order_relaxed) && afs.file_stream->IsActive()) {
                    const auto frames_read = readFloatFrames(afs.file_stream,
                        read_buffer,
                        channels,
                        static_cast<uint32_t>(kHopSize));
                    if (frames_read == 0) {
                        break;
                    }

                    shiftLeft(fft_input, kHopSize);
                    const auto write_offset = kFFTSize - kHopSize;
                    for (uint32_t frame = 0; frame < frames_read; ++frame) {
                        fft_input[write_offset + frame] = downmixFrame(
                            read_buffer.data() + frame * channels,
                            channels);
                    }

                    auto windowed_input = fft_input;
                    window(windowed_input.data(), windowed_input.size());
                    const auto& freq_bins = fft.Forward(windowed_input.data(), windowed_input.size());

                    const auto column = (std::min<int>)(
                        request.target_columns - 1,
                        static_cast<int>((processed_frames * request.target_columns) / total_frames));
                    for (int band = 0; band < bands; ++band) {
                        double db;
                        if ((band == 0) || (band == bands - 1)) {
                            db = getRealDb(freq_bins[band].real());
                        }
                        else {
                            db = getDb(freq_bins[band]);
                        }
                        db_sums[column * bands + band] += db;
                    }
                    ++counts[column];
                    processed_frames += frames_read;

                    const auto next_column = (std::min<int>)(
                        request.target_columns - 1,
                        static_cast<int>((processed_frames * request.target_columns) / total_frames));
                    if (next_column - emitted_column >= kChunkColumns) {
                        const auto chunk = makeSpectrogramChunk(color_table,
                            emitted_column,
                            next_column,
                            bands,
                            db_sums,
                            counts);
                        chunk_ready(emitted_column, chunk);
                        emitted_column = next_column;
                    }
                }

                if (!cancelled_.load(std::memory_order_relaxed) && emitted_column < request.target_columns) {
                    const auto chunk = makeSpectrogramChunk(color_table,
                        emitted_column,
                        request.target_columns,
                        bands,
                        db_sums,
                        counts);
                    chunk_ready(emitted_column, chunk);
                }
            }
            catch (const Exception& e) {
                error_ready(QString::fromUtf8(e.GetErrorMessage()));
            }
            catch (const std::exception& e) {
                error_ready(QString::fromUtf8(e.what()));
            }

            finished();
        }

    private:
        std::atomic_bool cancelled_{ false };
    };

}

SpectrogramWidget::SpectrogramWidget(QWidget *parent)
    : QFrame(parent) {
    setStyleSheet("background-color: transparent; border: none;"_str);
    setContextMenuPolicy(Qt::CustomContextMenu);

    color_ = qAppSettings.valueAsEnum<SpectrogramColor>(kAppSettingWaveformColor);
 
    (void)QObject::connect(this, &SpectrogramWidget::customContextMenuRequested, [this](auto pt) {
        ActionMap<SpectrogramWidget> action_map(this);

        auto *submenu = action_map.addSubMenu(tr("Change Spectrogram color"));
        submenu->addAction(tr("Default color"), [this]() {
            color_ = SPECTROGRAM_COLOR_DEFAULT;
            qAppSettings.setValue(kAppSettingWaveformColor, static_cast<int32_t>(color_));
            startReadSpectrogram(color_, file_path_);
            });

        submenu->addAction(tr("SoX color"), [this]() {
            color_ = SPECTROGRAM_COLOR_SOX;
            qAppSettings.setValue(kAppSettingWaveformColor, static_cast<int32_t>(color_));
            startReadSpectrogram(color_, file_path_);
            });

        const auto last_dir = qAppSettings.valueAsString(kAppSettingLastOpenFolderPath);
        const auto save_file_name = last_dir + "/"_str + "spectrogram.png"_str;
        action_map.addAction(tr("Save Spectrogram to image"), [this, save_file_name]() {
            getSaveFileName(this, [this](const QString& file_path) {
                if (file_path.isEmpty()) {
                    return;
                }
                if (static_cache_.isNull()) {
                    return;
                }
                if (!static_cache_.save(file_path, "PNG", 100)) {
                    XAMP_LOG_DEBUG("Failed to save spectrogram to {}", file_path.toStdString());
                }
                }, tr("Save Spectrogram to image"), save_file_name, "Image Files (*.png)"_str);
            });

        action_map.exec(pt);
        update();
        });
}

SpectrogramWidget::~SpectrogramWidget() {
    cancelReadSpectrogram();
}

void SpectrogramWidget::setCurrentPosition(float sec) {
	if (total_ms_ <= 0.f) {
		return;
	}
    cursor_ms_ = sec * 1000.f;
    update(drawRect());
}

void SpectrogramWidget::setTotalDuration(float duration) {
	total_ms_ = duration * 1000.f;
	update();
}

void SpectrogramWidget::setSampleRate(uint32_t sample_rate) {
	sample_rate_ = sample_rate;
	update();
}

void SpectrogramWidget::setDrawMode(uint32_t mode) {
    if (spectrogram_.isNull()) {
        startReadSpectrogram(color_, file_path_);
    }
	update();
}

void SpectrogramWidget::setSpectrogramData(double duration_sec, size_t hop_size, const QImage& chunk, int time_index) {
    if (time_index == 0) {
        setTotalDuration(static_cast<float>(duration_sec));
        spectrogram_ = makeImage(duration_sec, sample_rate_, hop_size, chunk);
        spectrogram_.fill(Qt::black);
    }

    QPainter p(&spectrogram_);
    p.drawImage(time_index, 0, chunk);
    p.end();   
}

void SpectrogramWidget::resizeSpectrogramSize() {
    if (spectrogram_.isNull())
        return;
    auto widget_size = drawRect().size();
    if (widget_size == spectrogram_cache_.size()) {
        return;
    }
    spectrogram_cache_ = spectrogram_.scaled(widget_size,
        Qt::KeepAspectRatioByExpanding);
}


QRect SpectrogramWidget::gainColorBarRect() const {
    // 先拿到頻譜主畫面範圍
    QRect waveRect = drawRect();

    // 設定色帶寬度和與頻譜之間的間隔
    int barWidth = 10;
    int spacing = 10; // 你想在頻譜和色帶之間留的空白

    // 設置頂、底與 waveRect 對齊
    int top = waveRect.top();
    int barHeight = waveRect.height();

    // 色帶的 left = waveRect.right() + spacing
    int left = waveRect.right() + spacing;

    // 回傳色帶位置
    return QRect(left, top, barWidth, barHeight);
}

QRect SpectrogramWidget::drawRect() const {
    constexpr int leftMargin = 60;
    constexpr int rightMargin = 80;
    constexpr int topMargin = 20;
    constexpr int bottomMargin = 20;

    QRect waveform_rect(leftMargin,
        topMargin,
        width() - leftMargin - rightMargin,
        height() - topMargin - bottomMargin);

    return waveform_rect;
}

void SpectrogramWidget::drawTimeAxis(QPainter& painter, const QRect& rect) {
    // 1) 取得音訊總秒數 (int)
    int total_sec = static_cast<int>(total_ms_ / 1000.f);
    if (total_sec <= 0) return;

    // 2) 設定字型、計算標籤寬度
    //    Spek 通常用 "00:00" 作為範例字串，測量文字長度。
    QFont f = painter.font();
    f.setPointSize(7); // 你可以調整字型大小
    painter.setFont(f);
    QFontMetrics fm(painter.font());

    // 假設我們用 "00:00" 來估計字串長度
    QString sample_label = "00:00"_str;
    int labelWidth = fm.horizontalAdvance(sample_label);
    // 給個倍數，確保標籤之間留足空間 (Spek 內用 ~1.5)
    double spacingFactor = 1.5;

    // 3) 準備 Spek 狀態下常用的 time factor (秒)
    //    這組清單與 Spek 幾乎一致: 
    //    1,2,5,10,20,30 秒、1分(60s)、2分(120s)、5分(300s)...等
    static const int time_factors[] = {
        1,2,5,10,20,30,
        60,120,300,600,1200,1800, // 1m,2m,5m,10m,20m,30m
        // 可再加更大, 3600(1h),7200(2h) ...
        0 // 結束標記
    };

    // 4) 計算每「1 秒」對應多少像素
    double scale = 0.0;
    if (total_sec > 0) {
        scale = double(rect.width()) / double(total_sec);
    }

    // 5) 從候選因子中選擇「最小可行」間距
    //    條件：相鄰標籤間距 >= spacingFactor * labelWidth
    //    => scale * factor >= spacingFactor * labelWidth
    //    => factor >= (spacingFactor * labelWidth) / scale
    int factor = 0;
    for (int i = 0; time_factors[i] != 0; ++i) {
        double pixelPerTick = scale * time_factors[i]; // 該刻度間距對應的畫布寬度
        if (pixelPerTick >= spacingFactor * labelWidth) {
            factor = time_factors[i];
            break;
        }
    }
    // 若都找不到 => total_sec 很大 => 就取最後1個大的 (例如 30分或1小時)
    if (factor == 0) {
        factor = 1800; // 你可改為更大的值，如 3600
    }

    // 6) 準備繪製：包含 0 與終點
    int min_units = 0;
    int max_units = total_sec;

    // 7) 開始畫軸
    int axisY = rect.bottom();
    painter.setPen(QPen(Qt::white, 1));

    // 先畫最左的 0秒 及最右的 max_units
    auto drawTickAndLabel = [&](int t) {
        // t秒 => x座標
        double ratio = double(t) / double(total_sec);
        if (ratio > 1.0) ratio = 1.0;
        float x_tick = float(rect.left() + ratio * rect.width());
        // 刻度線
        painter.drawLine(QPointF(x_tick, axisY),
            QPointF(x_tick, axisY + 4));
        // 標籤 mm:ss
        QString label = formatDurationAsMinutes(t);
        int tw = fm.horizontalAdvance(label);
        float x_text = x_tick - tw * 0.5f;
        float y_text = axisY + fm.height() + 2;
        painter.drawText(QPointF(x_text, y_text), label);
        };

    // 畫 0
    drawTickAndLabel(min_units);

    // 8) 中間刻度（factor倍數）。Spek 如果跟終點太近 => 跳出避免重疊
    for (int tick = min_units + factor; tick < max_units; tick += factor) {
        // 檢查是否離終點太近
        // => (max_units - tick)秒 * scale < labelWidth * 1.2
        double distToEndPx = (max_units - tick) * scale;
        if (distToEndPx < double(labelWidth) * 1.2) {
            break;
        }
        drawTickAndLabel(tick);
    }

    // 畫終點
    drawTickAndLabel(max_units);
}

void SpectrogramWidget::drawDuration(QPainter& painter, const QRect& rect) {
    if (cursor_ms_ < 0.f) {
        return;
    }

    float cur_sec = cursor_ms_ / 1000.f;
    float x_cursor = timeToX(cur_sec, rect);

    QFont f = painter.font();
    f.setPointSizeF(7.0f);
    painter.setFont(f);

    painter.setPen(QPen(QColor(100, 200, 255), 1));
    painter.drawLine(QPointF(x_cursor, rect.top() + 1),
        QPointF(x_cursor, rect.bottom()));

    QString time_text = formatDuration(cur_sec, false);

    const QFontMetrics fm(painter.font());
    int text_width = fm.horizontalAdvance(time_text);
    int text_height = fm.height();

    float box_w = text_width + kPadding * 2;
    float box_h = text_height + kPadding * 2;

    float lineGap = 1.0f;
    float x_text = x_cursor + lineGap;

    if (x_text + box_w > rect.right()) {
        x_text = x_cursor - lineGap - box_w;
    }

    if (x_text < rect.left()) {
        x_text = rect.left();
    }

    float y_box = rect.top() + 3.f;

    QRectF bg_rect(x_text, y_box, box_w, box_h);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(50, 50, 200, 200));
    painter.drawRoundedRect(bg_rect, kCornerRadius, kCornerRadius);

    // 9) 畫文字（讓文字置於框內，baseline 適度往下）
    painter.setPen(Qt::white);
    float text_baseline_y = y_box + kPadding + (text_height - fm.descent());
    float text_start_x = x_text + kPadding;
    painter.drawText(QPointF(text_start_x, text_baseline_y), time_text);
}

void SpectrogramWidget::drawFrequencyAxis(QPainter& painter, const QRect& rect) {
    // 1) 計算音檔的 Nyquist 頻率 = 取樣率的一半
    double nyquist = double(sample_rate_) * 0.5;
    if (nyquist <= 0.0) return;

    // 2) 設定字型，測量標籤寬度
    QFont f = painter.font();
    f.setPointSize(7); // 調整字型大小
    painter.setFont(f);
    QFontMetrics fm(painter.font());

    // Spek 可能用 "22.0 kHz" 當範例字串 => "22.0 kHz"
    // 你也可用 "99999 Hz" 之類，以確保最寬
    QString sample_label = "22 kHz"_str;
    int labelWidth = fm.horizontalAdvance(sample_label);
    double spacingFactor = 1.5; // 允許標籤間保留 1.5 個字寬

    // 3) 準備頻率刻度候選 (比時間多/大，從 1Hz ~ 20000Hz 之類)
    //    下例為簡化示例，可再增刪
    static const int freq_factors[] = {
        // 1,2,5,10,20,50,100,200,500, 
        1000,2000,3000,5000,10000,20000,
        0
    };
    // 你也可從較小開始: 1,2,5,10,20,50,100,200,500,1000,2000...
    // 若要更細，如 200, 400, 800... 可依需求添加

    // 4) 計算「1Hz 在畫面上佔幾個像素」
    //    這裡是「垂直軸」；所以 1Hz = rect.height() / nyquist
    //    取 linear scale => y = bottom - (freq/nyquist)*height
    double scale = 0.0;
    if (nyquist > 0.0) {
        scale = static_cast<double>(rect.height()) / nyquist;
    }

    // 5) 選擇「最小可行」刻度值 (freq step)
    int factor = 0;
    for (int i = 0; freq_factors[i] != 0; ++i) {
        double pixelPerTick = scale * freq_factors[i];
        if (pixelPerTick >= spacingFactor * labelWidth) {
            factor = freq_factors[i];
            break;
        }
    }
    // 如果一個都找不到 => 代表檔案取樣率極高 => 就取最後(20000)
    if (factor == 0) {
        factor = 20000;
    }

    // 6) 頻率軸通常畫在「rect.left()」左邊一點
    //    先畫一條主線
    const int axisRight = rect.left();
    painter.setPen(Qt::white);

    // 7) 幫你寫個小函式，用來繪製單一 tick
    auto drawTickAndLabel = [&](double freq) {
        if (freq < 0.0) freq = 0.0;
        if (freq > nyquist) freq = nyquist;

        double ratio = freq / nyquist;
        if (ratio > 1.0) ratio = 1.0;
        float y_tick = float(rect.bottom() - ratio * rect.height()) + 1;

        // 繪製小刻度
        painter.drawLine(QPointF(axisRight - 5, y_tick),
            QPointF(axisRight, y_tick));

        // 顯示文字: e.g. "2.0 kHz"
        // freq 若 < 1000 => 顯示 "123 Hz"
        // freq >= 1000 => 顯示 kHz
        QString label;
        if (freq < 999.5) {
            label = QString::number(freq, 'f', 0) + " Hz"_str;
        }
        else {
            double khz = freq * 1e-3;
            label = QString::number(khz, 'f', 0) + " kHz"_str;
        }
        int tw = fm.horizontalAdvance(label);
        float y_text = y_tick;

        // 左對齊
        painter.drawText(axisRight - 8 - tw, y_text, label);
        };

    // 8) 先畫0, 再畫中間刻度, 最後畫 nyquist
    drawTickAndLabel(0.0);

    // 中間刻度
    for (int tick = factor; tick < int(nyquist); tick += factor) {
        // Spek 會檢查是否離nyquist 太近 => 跳過 
        double dist_to_end_px = (nyquist - tick) * scale;
        if (dist_to_end_px < labelWidth * 1.2) {
            break;
        }
        drawTickAndLabel(tick);
    }

    // 再畫 nyquist
    drawTickAndLabel(nyquist);
}

void SpectrogramWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing
        | QPainter::SmoothPixmapTransform
        | QPainter::TextAntialiasing);

    if (cache_dirty_) {
        updateStaticCache();
    }

    painter.drawPixmap(0, 0, static_cache_);
    QRect waveform_rect = drawRect();
    drawDuration(painter, waveform_rect);
}

void SpectrogramWidget::updateStaticCache() {
    if (width() <= 0 || height() <= 0) {
        return;
    }

    // 準備一張跟 widget 一樣大小的影像
    qreal screenRatio = QGuiApplication::primaryScreen()->devicePixelRatio();
    QPixmap new_cache(width() * screenRatio, height() * screenRatio);    
    new_cache.fill(Qt::black);    
    new_cache.setDevicePixelRatio(devicePixelRatioF());

    QPainter p(&new_cache);
    p.setRenderHints(QPainter::Antialiasing
        | QPainter::SmoothPixmapTransform
        | QPainter::TextAntialiasing);

    // (1) 繪製背景顏色或底圖
    // 假設整個 widget 預設黑底
    // p.fillRect(new_cache.rect(), Qt::black);   

    // (2) 繪製已縮放好的頻譜圖 (spectrogram_cache_)
    QRect waveform_rect = drawRect();
    if (!spectrogram_cache_.isNull()) {
        p.drawImage(waveform_rect, spectrogram_cache_);
    }

    QPen framePen(Qt::white, 1);
    p.setPen(framePen);
    p.setBrush(Qt::NoBrush);
    p.drawRect(waveform_rect);

	QFont f = p.font();
	f.setPointSizeF(8.0f);
    p.setFont(f);

    // (3) 繪製頻率刻度、時間刻度等相對不常變動的UI
    drawFrequencyAxis(p, waveform_rect);
    drawTimeAxis(p, waveform_rect);

    QRect barRect = gainColorBarRect();
    drawGainColorBar(p, barRect);

    p.end();

    // 最後把這張繪好內容的 QImage 存到成員
    static_cache_ = new_cache;    
    cache_dirty_ = false; // 已更新
}

void SpectrogramWidget::drawGainColorBar(QPainter& painter, const QRect& barRect) {
    if (barRect.width() <= 0 || barRect.height() <= 0) {
        return;
    }

    ColorTable color_table;
    color_table.setSpectrogramColor(color_);

    QImage color_bar(barRect.size(), QImage::Format_RGB888);
    for (int y = 0; y < color_bar.height(); ++y) {
        const auto ratio = 1.0 - static_cast<double>(y) / (std::max)(1, color_bar.height() - 1);
        const auto db = ColorTable::kMinDb + ratio * ColorTable::kDbRange;
        const auto color = color_table[db];
        for (int x = 0; x < color_bar.width(); ++x) {
            color_bar.setPixel(x, y, color);
        }
    }

    painter.drawImage(barRect, color_bar);

    // 接著畫刻度
    drawGainColorBarTicks(painter, barRect);
}

void SpectrogramWidget::drawGainColorBarTicks(QPainter& painter, const QRect& barRect) {
    painter.save();
    painter.setPen(Qt::white);

    // 假設我們從 0 到 -120 dB，每隔 20 dB 一個刻度
    // 你可以把這組資料做成 array 以靈活加減
    int dbValues[] = { 0, -20, -40, -60, -80, -100, -120 };

    // barRect 的高度
    int barHeight = barRect.height();

    // 轉換：從 dB (0 ~ -120) 映射到 [0..1] -> 再對應到 barRect
    // 這裡假設 0 dB 對應 barRect.top()， -120 dB 對應 barRect.bottom().
    // ratio = (dB - minDB) / (maxDB - minDB)，因為 dB < 0，注意計算邏輯
    // 令 minDB = -120, maxDB = 0
    float minDB = -120.0f;
    float maxDB = 0.0f;
    auto dbToY = [&](float dbVal) {
        float ratio = (dbVal - minDB) / (maxDB - minDB);
        // 0 dB => ratio=1 -> y=barRect.top
        // -120 => ratio=0 -> y=barRect.bottom
        // 這樣 top -> bottom
        return (barRect.bottom() - ratio * barHeight) + 1;
        };

    QFontMetrics fm(painter.font());
    for (int dB : dbValues) {
        float y = dbToY(static_cast<float>(dB));
        // 畫刻度線(可選)
        painter.drawLine(QPointF(barRect.right() + 2, y), QPointF(barRect.right() + 4, y));

        // 畫文字
        QString label = QString("%1 dB"_str).arg(dB);
        int textWidth = fm.horizontalAdvance(label);
        // 假設文字畫在 bar 左邊 (barRect.left() - 8 - textWidth)
        painter.drawText(QPointF(barRect.right() + 8, y + fm.height() * 0.35f), label);
    }

    painter.restore();
}

float SpectrogramWidget::xToTime(float x, const QRect& rect) const {
    if (rect.width() <= 0) return 0.f;
    float total_sec = total_ms_ / 1000.f;
    if (total_sec <= 0.f) return 0.f;

    float localX = x - rect.left();
    if (localX < 0.f) localX = 0.f;
    if (localX > rect.width()) localX = static_cast<float>(rect.width());

    float ratio = localX / static_cast<float>(rect.width());
    float sec = ratio * total_sec;
    return sec;
}

float SpectrogramWidget::timeToX(float sec, const QRect& rect) const {
    if (total_ms_ <= 0.f) return static_cast<float>(rect.left());
    float total_sec = total_ms_ / 1000.f;
    if (total_sec <= 0.f) return static_cast<float>(rect.left());

    float ratio = sec / total_sec; // [0..1]
    if (ratio < 0.f) ratio = 0.f;
    if (ratio > 1.f) ratio = 1.f;

    return rect.left() + ratio * rect.width();
}

float SpectrogramWidget::mapFreqToY(float freq, const QRect& rect) const {
    float nyquist = static_cast<float>(sample_rate_) * 0.5f;
    if (nyquist <= 0.f) {
        return static_cast<float>(rect.bottom());
    }
    float ratio = freq / nyquist; // [0..1]
    if (ratio < 0.f) ratio = 0.f;
    if (ratio > 1.f) ratio = 1.f;

    float y = rect.bottom() - ratio * rect.height();
    return y;
}

void SpectrogramWidget::markCacheDirty() {
	cache_dirty_ = true;
}

void SpectrogramWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        QRect waveformRect = drawRect();
        float x = static_cast<float>(event->pos().x());
        float sec = xToTime(x, waveformRect);
		emit playAt(sec);
    }
}

void SpectrogramWidget::mouseMoveEvent(QMouseEvent* event) {
}

void SpectrogramWidget::mouseReleaseEvent(QMouseEvent* event) {
}

void SpectrogramWidget::resizeEvent(QResizeEvent* event) {
	QFrame::resizeEvent(event);
    if (file_path_.file_path.isEmpty() || is_processing_) {
        markCacheDirty();
        return;
    }
    const auto target_columns = (std::max)(1, drawRect().width());
    if (target_columns != spectrogram_columns_) {
        startReadSpectrogram(color_, file_path_);
        update();
        return;
    }
    resizeSpectrogramSize();
    markCacheDirty();
	update();
}

void SpectrogramWidget::doneRead() {
    XAMP_LOG_DEBUG("Done read!");
    resizeSpectrogramSize();
    spectrogram_ = QImage();
    is_processing_ = false;
    markCacheDirty();
    update();
}

void SpectrogramWidget::clear() {
    cancelReadSpectrogram();
    total_ms_ = 0.0f;
    cursor_ms_ = -1.f;
    spectrogram_columns_ = 0;
    spectrogram_ = QImage();
    spectrogram_cache_ = QImage();
    markCacheDirty();
    update();
}

void SpectrogramWidget::loadFile(const PlayListEntity& entity) {
	file_path_ = entity;
    startReadSpectrogram(color_, entity);
	update();
}

void SpectrogramWidget::startReadSpectrogram(SpectrogramColor color, const PlayListEntity& entity) {
    cancelReadSpectrogram();
    if (entity.file_path.isEmpty()) {
        return;
    }

    const auto load_id = spectrogram_load_id_;
    const auto target_columns = (std::max)(1, drawRect().width());
    spectrogram_columns_ = target_columns;
    is_processing_ = true;
    spectrogram_ = QImage();
    spectrogram_cache_ = QImage();
    markCacheDirty();
    update();

    auto* thread = new QThread(this);
    auto* worker = new SpectrogramWorker();
    worker->moveToThread(thread);
    spectrogram_thread_ = thread;
    spectrogram_worker_ = worker;

    (void)QObject::connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    (void)QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    (void)QObject::connect(thread, &QThread::finished, this, [this, thread]() {
        if (spectrogram_thread_ == thread) {
            spectrogram_thread_ = nullptr;
            spectrogram_worker_ = nullptr;
        }
    });

    const auto guard = QPointer<SpectrogramWidget>(this);
    SpectrogramReadRequest request{
        color,
        entity,
        target_columns
    };

    auto metadata_ready = [guard, load_id](double duration_sec,
        uint32_t sample_rate,
        int columns,
        int bands) {
        if (!guard) {
            return;
        }
        QMetaObject::invokeMethod(guard, [guard, load_id, duration_sec, sample_rate, columns, bands]() {
            if (!guard || guard->spectrogram_load_id_ != load_id) {
                return;
            }
            guard->setSampleRate(sample_rate);
            guard->setTotalDuration(static_cast<float>(duration_sec));
            guard->spectrogram_ = QImage(columns, bands, QImage::Format_RGB888);
            guard->spectrogram_.fill(Qt::black);
            guard->spectrogram_cache_ = QImage();
            guard->markCacheDirty();
            guard->update();
        }, Qt::QueuedConnection);
    };

    auto chunk_ready = [guard, load_id](int start_column, const QImage& chunk) {
        if (!guard) {
            return;
        }
        QMetaObject::invokeMethod(guard, [guard, load_id, start_column, chunk]() {
            if (!guard || guard->spectrogram_load_id_ != load_id || guard->spectrogram_.isNull()) {
                return;
            }
            QPainter painter(&guard->spectrogram_);
            painter.drawImage(start_column, 0, chunk);
            painter.end();
            guard->spectrogram_cache_ = QImage();
            guard->resizeSpectrogramSize();
            guard->markCacheDirty();
            guard->update();
        }, Qt::QueuedConnection);
    };

    auto error_ready = [guard, load_id](const QString& message) {
        if (!guard) {
            return;
        }
        QMetaObject::invokeMethod(guard, [guard, load_id, message]() {
            if (!guard || guard->spectrogram_load_id_ != load_id) {
                return;
            }
            XAMP_LOG_ERROR("{}", message.toStdString());
        }, Qt::QueuedConnection);
    };

    auto finished = [guard, thread, load_id]() {
        if (guard) {
            QMetaObject::invokeMethod(guard, [guard, load_id]() {
                if (guard && guard->spectrogram_load_id_ == load_id) {
                    guard->doneRead();
                }
            }, Qt::QueuedConnection);
        }
        thread->quit();
    };

    (void)QObject::connect(thread, &QThread::started, worker, [worker,
        request,
        metadata_ready = std::move(metadata_ready),
        chunk_ready = std::move(chunk_ready),
        error_ready = std::move(error_ready),
        finished = std::move(finished)]() mutable {
        worker->read(request,
            std::move(metadata_ready),
            std::move(chunk_ready),
            std::move(error_ready),
            std::move(finished));
    });

    thread->start(QThread::LowestPriority);
}

void SpectrogramWidget::cancelReadSpectrogram() {
    ++spectrogram_load_id_;
    if (spectrogram_worker_) {
        static_cast<SpectrogramWorker*>(spectrogram_worker_.data())->cancel();
    }
    if (spectrogram_thread_) {
        spectrogram_thread_->quit();
        spectrogram_thread_->wait();
        spectrogram_thread_ = nullptr;
        spectrogram_worker_ = nullptr;
    }
    is_processing_ = false;
}

