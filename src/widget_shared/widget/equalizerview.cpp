#include <widget/equalizerview.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <functional>
#include <limits>
#include <optional>

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

#include <thememanager.h>
#include <ui_equalizerdialog.h>
#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/util/str_util.h>
#include <widget/widget_shared.h>

#include <stream/bassparametriceq.h>
#include <stream/stft.h>
#include <base/rng.h>

namespace {
    constexpr double kMinFrequency = 20.0;
    constexpr double kMaxFrequency = 20000.0;
    constexpr double kMinDb = -15.0;
    constexpr double kMaxDb = 15.0;
    constexpr int32_t kHeaderRowHeight = 22;
    constexpr int32_t kBandControlHeight = 30;
    constexpr int32_t kBandScrollAreaMaxHeight = 420;
    constexpr int32_t kPreampScale = 10;
    constexpr int32_t kPreampWidgetWidth = 104;
    constexpr int32_t kPreampSliderHeight = 150;
    constexpr int32_t kPreampScaleWidth = 56;
    constexpr size_t kAnalyzerBuckets = 180;
    constexpr double kAnalyzerWindowCoherentGain = 0.54;
    constexpr double kAnalyzerDisplayOffsetDb = 24.0;
    constexpr double kAnalyzerSmoothing = 0.82;
    const auto kEqGraphPanelColor = "#303032"_str;

    const std::array<QColor, 10> kBandColors{
        QColor(213, 92, 93),
        QColor(210, 170, 87),
        QColor(169, 207, 91),
        QColor(85, 199, 97),
        QColor(92, 204, 169),
        QColor(88, 166, 204),
        QColor(99, 94, 210),
        QColor(195, 84, 207),
        QColor(215, 88, 88),
        QColor(218, 178, 87),
    };

    void AddCurveItems(QComboBox* combo_box) {
        combo_box->addItem(QStringLiteral("Bell"), static_cast<int32_t>(EQFilterTypes::FT_ALL_PEAKING_EQ));
        combo_box->addItem(QStringLiteral("Low-Shelf"), static_cast<int32_t>(EQFilterTypes::FT_LOW_SHELF));
        combo_box->addItem(QStringLiteral("High-Shelf"), static_cast<int32_t>(EQFilterTypes::FT_LOW_HIGH_SHELF));
        combo_box->addItem(QStringLiteral("High-Pass"), static_cast<int32_t>(EQFilterTypes::FT_HIGH_PASS));
        combo_box->addItem(QStringLiteral("Low-Pass"), static_cast<int32_t>(EQFilterTypes::FT_LOW_PASS));
        combo_box->addItem(QStringLiteral("Band-Pass"), static_cast<int32_t>(EQFilterTypes::FT_HIGH_BAND_PASS));
        combo_box->addItem(QStringLiteral("Band-Pass Q"), static_cast<int32_t>(EQFilterTypes::FT_HIGH_BAND_PASS_Q));
        combo_box->addItem(QStringLiteral("Notch"), static_cast<int32_t>(EQFilterTypes::FT_NOTCH));
        combo_box->addItem(QStringLiteral("All-Pass"), static_cast<int32_t>(EQFilterTypes::FT_ALL_PASS));
    }

    void SetCurveValue(QComboBox* combo_box, EQFilterTypes type) {
        if (type == EQFilterTypes::FT_UNKNOWN) {
            type = EQFilterTypes::FT_ALL_PEAKING_EQ;
        }
        const auto index = combo_box->findData(static_cast<int32_t>(type));
        combo_box->setCurrentIndex(index >= 0 ? index : 0);
    }

    QString BandColorStyle(const QColor& color) {
        return qFormat("background-color:%1; border-radius:6px;").arg(color.name());
    }

    QString FormatPreamp(float value) {
        return QStringLiteral("%1 dB").arg(value, 0, 'f', 1);
    }

    float DisplayGain(float band_gain, float preamp) {
        return std::clamp(band_gain + preamp,
            static_cast<float>(kEQMinDb),
            static_cast<float>(kEQMaxDb));
    }

    float BandGainFromDisplay(double display_gain, float preamp) {
        return std::clamp(static_cast<float>(display_gain) - preamp,
            static_cast<float>(kEQMinDb),
            static_cast<float>(kEQMaxDb));
    }

    double LogPosition(double frequency) {
        const auto safe_frequency = std::clamp(frequency, kMinFrequency, kMaxFrequency);
        return (std::log10(safe_frequency) - std::log10(kMinFrequency))
            / (std::log10(kMaxFrequency) - std::log10(kMinFrequency));
    }

    double FrequencyFromLogPosition(double position) {
        const auto value = std::clamp(position, 0.0, 1.0);
        return std::pow(10.0, std::log10(kMinFrequency) + value * (std::log10(kMaxFrequency) - std::log10(kMinFrequency)));
    }

    double GainFromPosition(const QRectF& plot, double y) {
        const auto position = std::clamp((plot.bottom() - y) / plot.height(), 0.0, 1.0);
        return kMinDb + position * (kMaxDb - kMinDb);
    }

    double SmoothStep(double value) {
        const auto x = std::clamp(value, 0.0, 1.0);
        return x * x * (3.0 - 2.0 * x);
    }

    double BandResponse(const EqBandSetting& band, double frequency) {
        const auto center = std::clamp(static_cast<double>(band.frequency), kMinFrequency, kMaxFrequency);
        const auto q = std::max(0.1, static_cast<double>(band.Q));
        const auto octave = std::log2(frequency / center);

        switch (band.type) {
        case EQFilterTypes::FT_LOW_PASS:
            return -24.0 * SmoothStep(std::log2(frequency / center) / 2.0);
        case EQFilterTypes::FT_HIGH_PASS:
            return -24.0 * SmoothStep(std::log2(center / frequency) / 2.0);
        case EQFilterTypes::FT_LOW_SHELF:
            return static_cast<double>(band.gain) / (1.0 + std::pow(frequency / center, q * 2.0));
        case EQFilterTypes::FT_LOW_HIGH_SHELF:
            return static_cast<double>(band.gain) / (1.0 + std::pow(center / frequency, q * 2.0));
        case EQFilterTypes::FT_NOTCH:
            return -std::max(6.0, std::abs(static_cast<double>(band.gain))) * std::exp(-(octave * octave) * q * 2.0);
        case EQFilterTypes::FT_ALL_PASS:
            return 0.0;
        case EQFilterTypes::FT_HIGH_BAND_PASS:
        case EQFilterTypes::FT_HIGH_BAND_PASS_Q:
            return 3.0 * std::exp(-(octave * octave) * q * 2.0);
        case EQFilterTypes::FT_UNKNOWN:
        case EQFilterTypes::FT_ALL_PEAKING_EQ:
        default:
            return static_cast<double>(band.gain) * std::exp(-(octave * octave) * q);
        }
    }

    void ClearLayout(QLayout* layout) {
        while (auto* item = layout->takeAt(0)) {
            delete item->widget();
            delete item;
        }
    }

    QLabel* MakeHeaderLabel(QWidget* parent, const QString& text, Qt::Alignment alignment) {
        auto* label = new QLabel(text, parent);
        auto font = qTheme.monoFont();
        font.setPointSize(qTheme.fontSize(8));
        label->setFont(font);
        label->setFixedHeight(kHeaderRowHeight);
        label->setAlignment(alignment);
        label->setStyleSheet(QStringLiteral("color:#f5f5f5; background-color: transparent;"));
        return label;
    }
}

class ParametricEqGraph final : public QWidget {
public:
    explicit ParametricEqGraph(QWidget* parent)
        : QWidget(parent) {
        setMinimumHeight(220);
        setMaximumHeight(300);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setMouseTracking(true);
    }

    void setSettings(const EqSettings& settings, const std::vector<bool>& enabled) {
        settings_ = settings;
        enabled_ = enabled;
        update();
    }

    void setBandMovedCallback(std::function<void(size_t, float, float)> callback) {
        band_moved_callback_ = std::move(callback);
    }

    void setAnalyzerSpectrum(ComplexValarray spectrum, int32_t sample_rate, size_t frame_size) {
        analyzer_spectrum_ = std::move(spectrum);
        analyzer_sample_rate_ = sample_rate;
        analyzer_frame_size_ = frame_size;
        update();
    }

    void clearAnalyzerSpectrum() {
        analyzer_spectrum_.clear();
        analyzer_smoothed_db_.fill(-std::numeric_limits<double>::infinity());
        analyzer_smoothed_ready_ = false;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        const auto background = QColor(48, 48, 50);
        const auto grid = QColor(255, 255, 255, 48);
        const auto axis = QColor(255, 255, 255, 145);
        const auto text = QColor(245, 245, 245);
        const auto curve = QColor(47, 173, 236);

        painter.fillRect(rect(), background);

        const auto plot = plotRect();

        painter.setFont(qTheme.monoFont());
        painter.setPen(QPen(grid, 1));

        for (auto db = -15; db <= 15; db += 3) {
            const auto y = yFromDb(plot, db);
            painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
            painter.setPen(text);
            painter.drawText(QRectF(6, y - 9, 42, 18), Qt::AlignRight | Qt::AlignVCenter, QString::number(db));
            painter.setPen(QPen(grid, 1));
        }

        for (const auto frequency : { 30, 40, 50, 70, 90, 120, 170, 300, 500, 700, 1000, 1500, 3000, 5000, 8000, 12000, 16000 }) {
            const auto x = xFromFrequency(plot, frequency);
            painter.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
        }

        painter.setPen(QPen(axis, 1.4));
        painter.drawLine(QPointF(plot.left(), yFromDb(plot, 0)), QPointF(plot.right(), yFromDb(plot, 0)));

        painter.setPen(text);
        painter.drawText(QRectF(0, plot.center().y() - 12, 30, 24), Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("dB"));
        painter.drawText(QRectF(plot.center().x() - 28, plot.bottom() + 30, 56, 22), Qt::AlignCenter, QStringLiteral("Hz"));

        for (const auto frequency : { 40, 50, 70, 90, 120, 170, 300, 500, 700, 1000, 1500, 3000, 5000, 8000, 12000 }) {
            const auto x = xFromFrequency(plot, frequency);
            painter.drawText(QRectF(x - 24, plot.bottom() + 6, 48, 18), Qt::AlignCenter, QString::number(frequency));
        }

        if (!analyzer_spectrum_.empty() && analyzer_sample_rate_ > 0 && analyzer_frame_size_ > 0) {
            QPainterPath analyzer_path;

            std::array<double, kAnalyzerBuckets + 1> analyzer_db{};
            analyzer_db.fill(-std::numeric_limits<double>::infinity());

            for (auto bucket = size_t{ 0 }; bucket < analyzer_db.size(); ++bucket) {
                const auto left_t = bucket == 0
                    ? 0.0
                    : (static_cast<double>(bucket) - 0.5) / kAnalyzerBuckets;
                const auto right_t = bucket == kAnalyzerBuckets
                    ? 1.0
                    : (static_cast<double>(bucket) + 0.5) / kAnalyzerBuckets;
                const auto left_frequency = FrequencyFromLogPosition(left_t);
                const auto right_frequency = FrequencyFromLogPosition(right_t);
                auto power = 0.0;
                auto count = size_t{ 0 };

                const auto first_bin = std::max<size_t>(
                    1,
                    static_cast<size_t>(std::floor(left_frequency * analyzer_frame_size_ / analyzer_sample_rate_)));
                const auto last_bin = std::min(
                    analyzer_spectrum_.size() - 1,
                    static_cast<size_t>(std::ceil(right_frequency * analyzer_frame_size_ / analyzer_sample_rate_)));

                if (first_bin > last_bin) {
                    continue;
                }

                for (auto bin = first_bin; bin <= last_bin; ++bin) {
                    const auto frequency = static_cast<double>(bin) * analyzer_sample_rate_
                        / static_cast<double>(analyzer_frame_size_);
                    if (frequency < kMinFrequency || frequency > kMaxFrequency) {
                        continue;
                    }
                    const auto magnitude = static_cast<double>(std::abs(analyzer_spectrum_[bin]));
                    power += magnitude * magnitude;
                    ++count;
                }

                if (count > 0 && power > 0.0) {
                    const auto bucket_energy = std::sqrt(power);
                    const auto normalized_energy = bucket_energy * 2.0
                        / (static_cast<double>(analyzer_frame_size_) * kAnalyzerWindowCoherentGain);
                    analyzer_db[bucket] = 20.0 * std::log10(normalized_energy + 1e-8)
                        + kAnalyzerDisplayOffsetDb;
                }
            }

            auto started = false;
            for (auto bucket = size_t{ 0 }; bucket < analyzer_db.size(); ++bucket) {
                if (!std::isfinite(analyzer_db[bucket])) {
                    continue;
                }
                if (!analyzer_smoothed_ready_ || !std::isfinite(analyzer_smoothed_db_[bucket])) {
                    analyzer_smoothed_db_[bucket] = analyzer_db[bucket];
                }
                else {
                    analyzer_smoothed_db_[bucket] = analyzer_smoothed_db_[bucket] * kAnalyzerSmoothing
                        + analyzer_db[bucket] * (1.0 - kAnalyzerSmoothing);
                }
                const auto t = static_cast<double>(bucket) / kAnalyzerBuckets;
                const auto frequency = FrequencyFromLogPosition(t);
                const auto db = std::clamp(analyzer_smoothed_db_[bucket], kMinDb, kMaxDb);
                const QPointF point(xFromFrequency(plot, frequency), yFromDb(plot, db));
                if (!started) {
                    analyzer_path.moveTo(point);
                    started = true;
                }
                else {
                    analyzer_path.lineTo(point);
                }
            }
            analyzer_smoothed_ready_ = started;

            if (started) {
                painter.setPen(QPen(QColor(235, 235, 235, 82), 1.2));
                painter.drawPath(analyzer_path);
            }
        }

        QPainterPath path;
        constexpr auto kSteps = 260;
        for (auto i = 0; i <= kSteps; ++i) {
            const auto t = static_cast<double>(i) / kSteps;
            const auto frequency = std::pow(10.0, std::log10(kMinFrequency) + t * (std::log10(kMaxFrequency) - std::log10(kMinFrequency)));
            auto db = 0.0;
            for (auto band = 0; band < settings_.bands.size(); ++band) {
                if (band < enabled_.size() && !enabled_[band]) {
                    continue;
                }
                db += BandResponse(settings_.bands[band], frequency);
            }
            db = std::clamp(db + static_cast<double>(settings_.preamp), -30.0, 30.0);
            const QPointF point(plot.left() + plot.width() * t, yFromDb(plot, db));
            if (i == 0) {
                path.moveTo(point);
            }
            else {
                path.lineTo(point);
            }
        }

        painter.setPen(QPen(curve, 3));
        painter.drawPath(path);

        for (auto band = 0; band < settings_.bands.size(); ++band) {
            if (band < enabled_.size() && !enabled_[band]) {
                continue;
            }
            const auto& setting = settings_.bands[band];
            const auto color = kBandColors[band % kBandColors.size()];
            const auto x = xFromFrequency(plot, setting.frequency);
            const auto y = yFromDb(plot, setting.gain + settings_.preamp);
            painter.setBrush(color);
            painter.setPen(QPen(background, 1.5));
            painter.drawEllipse(QPointF(x, y), 7, 7);
        }
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() != Qt::LeftButton) {
            QWidget::mousePressEvent(event);
            return;
        }

        dragging_band_ = hitTestBand(event->position());
        if (dragging_band_) {
            setCursor(Qt::ClosedHandCursor);
            moveBandTo(event->position());
            event->accept();
            return;
        }
        QWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (dragging_band_) {
            moveBandTo(event->position());
            event->accept();
            return;
        }

        setCursor(hitTestBand(event->position()) ? Qt::OpenHandCursor : Qt::ArrowCursor);
        QWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton && dragging_band_) {
            moveBandTo(event->position());
            dragging_band_.reset();
            setCursor(Qt::OpenHandCursor);
            event->accept();
            return;
        }
        QWidget::mouseReleaseEvent(event);
    }

    void leaveEvent(QEvent* event) override {
        if (!dragging_band_) {
            unsetCursor();
        }
        QWidget::leaveEvent(event);
    }

private:
    QRectF plotRect() const {
        return rect().adjusted(68, 14, -20, -58);
    }

    double xFromFrequency(const QRectF& plot, double frequency) const {
        return plot.left() + plot.width() * LogPosition(frequency);
    }

    double yFromDb(const QRectF& plot, double db) const {
        const auto value = std::clamp(db, kMinDb, kMaxDb);
        return plot.bottom() - plot.height() * ((value - kMinDb) / (kMaxDb - kMinDb));
    }

    QPointF bandPoint(size_t band) const {
        const auto plot = plotRect();
        const auto& setting = settings_.bands[band];
        return QPointF(xFromFrequency(plot, setting.frequency), yFromDb(plot, setting.gain + settings_.preamp));
    }

    std::optional<size_t> hitTestBand(const QPointF& position) const {
        constexpr auto kHitRadius = 13.0;
        for (auto band = settings_.bands.size(); band > 0; --band) {
            const auto index = band - 1;
            if (index < enabled_.size() && !enabled_[index]) {
                continue;
            }
            const auto point = bandPoint(index);
            const auto distance = std::hypot(point.x() - position.x(), point.y() - position.y());
            if (distance <= kHitRadius) {
                return index;
            }
        }
        return std::nullopt;
    }

    void moveBandTo(const QPointF& position) {
        if (!dragging_band_ || *dragging_band_ >= settings_.bands.size()) {
            return;
        }

        const auto plot = plotRect();
        const auto clamped_x = std::clamp(position.x(), plot.left(), plot.right());
        const auto clamped_y = std::clamp(position.y(), plot.top(), plot.bottom());
        const auto frequency = FrequencyFromLogPosition((clamped_x - plot.left()) / plot.width());
        const auto gain = GainFromPosition(plot, clamped_y) - static_cast<double>(settings_.preamp);

        auto& setting = settings_.bands[*dragging_band_];
        setting.frequency = static_cast<float>(std::clamp(frequency, kMinFrequency, kMaxFrequency));
        setting.gain = static_cast<float>(std::clamp(gain, static_cast<double>(kEQMinDb), static_cast<double>(kEQMaxDb)));

        if (band_moved_callback_) {
            band_moved_callback_(*dragging_band_, setting.frequency, setting.gain);
        }
        update();
    }

    EqSettings settings_;
    std::vector<bool> enabled_;
    ComplexValarray analyzer_spectrum_;
    std::array<double, kAnalyzerBuckets + 1> analyzer_smoothed_db_{};
    int32_t analyzer_sample_rate_{ 0 };
    size_t analyzer_frame_size_{ 0 };
    bool analyzer_smoothed_ready_{ false };
    std::optional<size_t> dragging_band_;
    std::function<void(size_t, float, float)> band_moved_callback_;
};

class PreampScaleWidget final : public QWidget {
public:
    explicit PreampScaleWidget(QWidget* parent)
        : QWidget(parent) {
        setFixedSize(kPreampScaleWidth, kPreampSliderHeight);
        setAttribute(Qt::WA_TranslucentBackground);
        setAutoFillBackground(false);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        auto font = qTheme.monoFont();
        font.setPointSize(qTheme.fontSize(8));
        painter.setFont(font);

        const auto tick_color = QColor(245, 245, 245, 90);
        const auto label_color = QColor(245, 245, 245, 180);
        constexpr auto tick_left = 2;
        constexpr auto label_left = 14;
        constexpr auto label_height = 18;

        painter.setPen(QPen(tick_color, 1));
        for (auto db = kEQMinDb; db <= kEQMaxDb; db += 3) {
            const auto y = yFromDb(db);
            const auto tick_width = db == 0 || db == kEQMinDb || db == kEQMaxDb ? 5 : 3;
            painter.drawLine(tick_left, y, tick_left + tick_width, y);
        }

        painter.setPen(label_color);
        painter.drawText(QRectF(label_left, yFromDb(kEQMaxDb) - label_height / 2.0, width() - label_left - 2, label_height),
            Qt::AlignLeft | Qt::AlignVCenter,
            QStringLiteral("+15"));
        painter.drawText(QRectF(label_left, yFromDb(0) - label_height / 2.0, width() - label_left - 2, label_height),
            Qt::AlignLeft | Qt::AlignVCenter,
            QStringLiteral("0"));
        painter.drawText(QRectF(label_left, yFromDb(kEQMinDb) - label_height / 2.0, width() - label_left - 2, label_height),
            Qt::AlignLeft | Qt::AlignVCenter,
            QStringLiteral("-15"));
    }

private:
    double yFromDb(double db) const {
        constexpr auto vertical_padding = 10.0;
        const auto position = (static_cast<double>(kEQMaxDb) - db)
            / static_cast<double>(kEQMaxDb - kEQMinDb);
        return vertical_padding
            + std::clamp(position, 0.0, 1.0) * static_cast<double>(height() - vertical_padding * 2.0);
    }
};

EqualizerView::EqualizerView(QWidget* parent)
    : QFrame(parent) {
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    ui_ = new Ui::EqualizerView();
    ui_->setupUi(this);

    graph_ = new ParametricEqGraph(this);
    graph_->setBandMovedCallback([this](size_t band, float frequency, float gain) {
        updateBandFromGraph(band, frequency, gain);
        });

    auto* graph_row_layout = new QHBoxLayout();
    graph_row_layout->setSpacing(4);
    graph_row_layout->setContentsMargins(0, 0, 0, 0);
    graph_row_layout->addWidget(graph_, 1);

    auto* preamp_widget = new QWidget(this);
    preamp_widget->setObjectName(QStringLiteral("preampWidget"));
    preamp_widget->setFixedWidth(kPreampWidgetWidth);
    preamp_widget->setAttribute(Qt::WA_TranslucentBackground);
    preamp_widget->setAutoFillBackground(false);
    auto* preamp_layout = new QVBoxLayout(preamp_widget);
    preamp_layout->setSpacing(4);
    preamp_layout->setContentsMargins(0, 2, 0, 2);

    auto preamp_font = qTheme.monoFont();
    preamp_font.setPointSize(qTheme.fontSize(8));
    preamp_enabled_checkbox_ = new QCheckBox(QStringLiteral("Preamp"), preamp_widget);
    preamp_enabled_checkbox_->setChecked(true);
    preamp_enabled_checkbox_->setFont(preamp_font);
    preamp_enabled_checkbox_->setFixedHeight(18);
    preamp_enabled_checkbox_->setToolTip(QStringLiteral("Enable preamp for this EQ"));

    preamp_value_label_ = new QLabel(preamp_widget);
    preamp_value_label_->setFont(preamp_font);
    preamp_value_label_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    preamp_value_label_->setFixedHeight(20);

    preamp_slider_ = new QSlider(Qt::Vertical, preamp_widget);
    preamp_slider_->setRange(kEQMinDb * kPreampScale, kEQMaxDb * kPreampScale);
    preamp_slider_->setSingleStep(1);
    preamp_slider_->setPageStep(10);
    preamp_slider_->setTickInterval(30);
    preamp_slider_->setTickPosition(QSlider::NoTicks);
    preamp_slider_->setFixedSize(24, kPreampSliderHeight);

    auto* preamp_scale = new PreampScaleWidget(preamp_widget);
    auto* preamp_slider_layout = new QHBoxLayout();
    preamp_slider_layout->setSpacing(2);
    preamp_slider_layout->setContentsMargins(0, 0, 0, 0);
    preamp_slider_layout->addWidget(preamp_slider_, 0, Qt::AlignLeft | Qt::AlignVCenter);
    preamp_slider_layout->addWidget(preamp_scale, 0, Qt::AlignLeft | Qt::AlignVCenter);

    preamp_layout->addWidget(preamp_enabled_checkbox_);
    preamp_layout->addLayout(preamp_slider_layout, 1);
    preamp_layout->addWidget(preamp_value_label_);
    graph_row_layout->addWidget(preamp_widget);

    ui_->graphLayout->addLayout(graph_row_layout);
    ui_->graphFrame->setMaximumHeight(300);
    ui_->graphFrame->setStyleSheet(qFormat("background-color:%1; border:none;").arg(kEqGraphPanelColor));
    ui_->bandsScrollArea->setFocusPolicy(Qt::NoFocus);
    ui_->bandsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui_->bandsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui_->bandsScrollArea->viewport()->setStyleSheet(qFormat("background-color:%1;").arg(qTheme.backgroundColorString()));
    ui_->bandsScrollAreaWidget->setStyleSheet(qFormat("background-color:%1;").arg(qTheme.backgroundColorString()));

    apply_button_ = new QPushButton(QStringLiteral("Apply"), this);
    apply_button_->setToolTip(QStringLiteral("Apply current EQ settings to playback"));
    ui_->toolbarLayout->insertWidget(3, apply_button_);

    test_wave_button_ = new QPushButton(QStringLiteral("Test Wave"), this);
    test_wave_button_->setToolTip(QStringLiteral("Generate pink noise for the analyzer"));
    ui_->toolbarLayout->insertWidget(4, test_wave_button_);

    test_wave_timer_ = new QTimer(this);
    test_wave_timer_->setInterval(100);
    test_wave_timer_->setTimerType(Qt::CoarseTimer);

    setStyleSheet(qFormat(R"(
        QFrame#EqualizerView {
            background-color: %1;
        }
        QLabel {
            color: #f5f5f5;
            background-color: transparent;
        }
        QComboBox, QDoubleSpinBox, QSpinBox {
            color: #f8f8f8;
            background-color: #252525;
            border: 1px solid #383838;
            border-radius: 6px;
            padding: 2px 8px;
            min-height: 22px;
        }
        QPushButton {
            color: #f8f8f8;
            background-color: #242424;
            border: 1px solid #3b3b3b;
            border-radius: 6px;
            padding: 6px 14px;
        }
        QPushButton:hover {
            background-color: #303030;
        }
        QScrollArea {
            background-color: %1;
            border: none;
        }
        QScrollArea#bandsScrollArea {
            background-color: %1;
            border: none;
        }
        QWidget#bandsScrollAreaWidget {
            background-color: %1;
        }
        QScrollBar:horizontal {
            height: 0px;
            background: transparent;
            border: none;
        }
        QScrollBar:vertical {
            width: 8px;
            background: #2a2a2a;
            border: none;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: #555555;
            border-radius: 4px;
            min-height: 24px;
        }
        QScrollBar::handle:vertical:hover {
            background: #686868;
        }
        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical,
        QScrollBar::add-page:vertical,
        QScrollBar::sub-page:vertical {
            background: transparent;
            border: none;
            height: 0px;
        }
        QCheckBox {
            color: #f8f8f8;
            background-color: transparent;
        }
        QWidget#preampWidget {
            background-color: %2;
            border: none;
        }
        QSlider::groove:vertical {
            background-color: #202020;
            border: 1px solid #454545;
            border-radius: 3px;
            width: 5px;
        }
        QSlider::sub-page:vertical {
            background-color: #3a3a3a;
            border-radius: 3px;
        }
        QSlider::add-page:vertical {
            background-color: #6f65d6;
            border-radius: 3px;
        }
        QSlider::handle:vertical {
            background-color: #f5f5f5;
            border: 2px solid #2f2f2f;
            height: 13px;
            margin: 0px -6px;
            border-radius: 7px;
        }
    )").arg(qTheme.backgroundColorString(), kEqGraphPanelColor));

    (void)QObject::connect(preamp_slider_, &QSlider::valueChanged, [this](int value) {
        current_settings_.preamp = static_cast<float>(value) / kPreampScale;
        preamp_value_label_->setText(FormatPreamp(current_settings_.preamp));
        emit preampValueChanged(current_settings_.preamp);
        updateBandGainControls();
        updateGraph();
        });

    (void)QObject::connect(preamp_enabled_checkbox_, &QCheckBox::stateChanged, [this](auto) {
        const auto enabled = preamp_enabled_checkbox_->isChecked();
        preamp_slider_->setEnabled(enabled);
        preamp_value_label_->setEnabled(enabled);
        updateBandGainControls();
        updateGraph();
        });

    (void)QObject::connect(apply_button_, &QPushButton::clicked, [this]() {
        emit parametricEqChanged(ui_->enableEqCheckBox->isChecked(), currentEnabledSettings());
        });

    (void)QObject::connect(test_wave_button_, &QPushButton::clicked, this, &EqualizerView::toggleTestWaveformGeneration);
    (void)QObject::connect(test_wave_timer_, &QTimer::timeout, this, &EqualizerView::generateTestWaveform);

    (void)QObject::connect(ui_->enableEqCheckBox, &QCheckBox::stateChanged, [this](auto value) {
        const auto enabled = value == Qt::CheckState::Checked;
        qAppSettings.setValue(kAppSettingEnableEQ, enabled);
        qAppSettings.save();
        });

    (void)QObject::connect(ui_->resetButton, &QPushButton::clicked, [this]() {
        for (auto& band : current_settings_.bands) {
            band.gain = 0;
            if (band.Q <= 0) {
                band.Q = kDefaultQ;
            }
        }
        current_settings_.preamp = 0;
        applySetting(current_name_, current_settings_);
        saveCurrentSetting();
        });

    (void)QObject::connect(ui_->saveButton, &QPushButton::clicked, [this]() {
        saveCurrentSetting();
        });

    (void)QObject::connect(ui_->eqPresetComboBox, &QComboBox::textActivated, [this](const auto& name) {
        qAppSettings.loadEqPreset();
        auto settings = qAppSettings.eqPreset().value(name);
        if (settings.bands.empty()) {
            settings.SetDefault();
        }
        applySetting(name, settings);
        saveCurrentSetting();
        });

    ui_->enableEqCheckBox->setCheckState(qAppSettings.valueAsBool(kAppSettingEnableEQ)
        ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);

    for (const auto& name : qAppSettings.eqPreset().keys()) {
        ui_->eqPresetComboBox->addItem(name);
    }

    QString name = QStringLiteral("Manual");
    EqSettings settings;

    if (qAppSettings.contains(kAppSettingEQName)) {
        const auto app_settings = qAppSettings.eqSettings();
        name = app_settings.name;
        settings = app_settings.settings;
        const auto preset_settings = qAppSettings.eqPreset().value(name);
        if (name != QStringLiteral("Manual") && !preset_settings.bands.empty()) {
            settings = preset_settings;
            settings.preamp = app_settings.settings.preamp;
        }
    }

    if (settings.bands.empty()) {
        settings = qAppSettings.eqPreset().value(name);
    }
    if (settings.bands.empty()) {
        settings.SetDefault();
    }

    applySetting(name, settings);
}

EqualizerView::~EqualizerView() {
    delete ui_;
}

void EqualizerView::outputFormatChanged(int32_t sample_rate, size_t) {
    if (analyzer_sample_rate_ == sample_rate) {
        return;
    }
    analyzer_sample_rate_ = sample_rate;
    resetAnalyzer();
    resetTestEq();
}

void EqualizerView::samplesChanged(std::vector<float> samples, size_t num_samples) {
    if (samples.empty() || analyzer_sample_rate_ <= 0) {
        return;
    }

    if (num_samples < samples.size()) {
        samples.resize(num_samples);
    }

    configureAnalyzer(samples.size());
    if (analyzer_stft_ == nullptr) {
        return;
    }

    graph_->setAnalyzerSpectrum(
        analyzer_stft_->Process(samples.data(), samples.size()),
        analyzer_sample_rate_,
        analyzer_frame_size_);
}

void EqualizerView::resetAnalyzer() {
    analyzer_stft_.reset();
    analyzer_frame_size_ = 0;
    analyzer_shift_size_ = 0;
    if (graph_ != nullptr) {
        graph_->clearAnalyzerSpectrum();
    }
}

void EqualizerView::resetTestEq() {
    test_eq_.reset();
    test_eq_buffer_.reset();
    test_eq_sample_rate_ = 0;
}

void EqualizerView::configureAnalyzer(size_t num_samples) {
    const auto sample_frames = num_samples / AudioFormat::kMaxChannel;
    if (sample_frames == 0) {
        resetAnalyzer();
        return;
    }

    //auto frame_size = size_t{ 4096 };
    auto frame_size = size_t{ 8192 };
    while (frame_size <= sample_frames && frame_size < 32768) {
        frame_size *= 2;
    }

    if (sample_frames >= frame_size) {
        resetAnalyzer();
        return;
    }

    const auto shift_size = sample_frames;
    if (analyzer_stft_ != nullptr
        && analyzer_frame_size_ == frame_size
        && analyzer_shift_size_ == shift_size) {
        return;
    }

    analyzer_frame_size_ = frame_size;
    analyzer_shift_size_ = shift_size;
    analyzer_stft_ = xamp::base::MakeAlign<xamp::stream::STFT>(analyzer_frame_size_, analyzer_shift_size_);
    analyzer_stft_->SetWindowType(WindowType::HAMMING);
}

void EqualizerView::toggleTestWaveformGeneration() {
    if (test_wave_timer_ == nullptr || test_wave_button_ == nullptr) {
        return;
    }

    if (test_wave_timer_->isActive()) {
        test_wave_timer_->stop();
        test_wave_button_->setText(QStringLiteral("Test Wave"));
        test_wave_button_->setToolTip(QStringLiteral("Generate pink noise for the analyzer"));
        return;
    }

    test_noise_state_.clear();
    test_wave_button_->setText(QStringLiteral("Stop generate"));
    test_wave_button_->setToolTip(QStringLiteral("Stop generating pink noise"));
    generateTestWaveform();
    test_wave_timer_->start();
}

void EqualizerView::generateTestWaveform() {
    if (analyzer_sample_rate_ <= 0) {
        analyzer_sample_rate_ = AudioFormat::kFloatPCM48Khz.GetSampleRate();
        resetAnalyzer();
        resetTestEq();
    }

    if (test_noise_state_.size() != 7) {
        test_noise_state_.assign(7, 0.0);
    }

    constexpr auto kTestSampleFrames = size_t{ 4096 };
    auto& prng = PRNG::GetThreadLocal();
    std::vector<float> samples(kTestSampleFrames * AudioFormat::kMaxChannel);
    for (auto frame = size_t{ 0 }; frame < kTestSampleFrames; ++frame) {
        const auto white = static_cast<double>(prng.NextSingle(-1.0f, 1.0f));
        test_noise_state_[0] = 0.99886 * test_noise_state_[0] + white * 0.0555179;
        test_noise_state_[1] = 0.99332 * test_noise_state_[1] + white * 0.0750759;
        test_noise_state_[2] = 0.96900 * test_noise_state_[2] + white * 0.1538520;
        test_noise_state_[3] = 0.86650 * test_noise_state_[3] + white * 0.3104856;
        test_noise_state_[4] = 0.55000 * test_noise_state_[4] + white * 0.5329522;
        test_noise_state_[5] = -0.7616 * test_noise_state_[5] - white * 0.0168980;

        auto pink = test_noise_state_[0]
            + test_noise_state_[1]
            + test_noise_state_[2]
            + test_noise_state_[3]
            + test_noise_state_[4]
            + test_noise_state_[5]
            + test_noise_state_[6]
            + white * 0.5362;
        test_noise_state_[6] = white * 0.115926;

        const auto sample = static_cast<float>(std::clamp(pink * 0.11, -0.95, 0.95));
        samples[frame * AudioFormat::kMaxChannel] = sample;
        samples[frame * AudioFormat::kMaxChannel + 1] = sample;
    }

    applyTestEq(samples);
    samplesChanged(std::move(samples), kTestSampleFrames * AudioFormat::kMaxChannel);
}

void EqualizerView::configureTestEq(const EqSettings& settings) {
    if (analyzer_sample_rate_ <= 0) {
        return;
    }

    if (test_eq_ == nullptr || test_eq_sample_rate_ != analyzer_sample_rate_) {
        auto output_format = AudioFormat::kFloatPCM48Khz;
        output_format.SetSampleRate(static_cast<uint32_t>(analyzer_sample_rate_));

        Property config;
        config.Create(DspConfig::kOutputFormat, output_format);
        config.Create(DspConfig::kEQSettings, settings);

        test_eq_ = xamp::base::MakeAlign<xamp::stream::BassParametricEq>();
        test_eq_->Initialize(config);
        test_eq_sample_rate_ = analyzer_sample_rate_;
        return;
    }

    test_eq_->SetEq(settings);
}

void EqualizerView::applyTestEq(std::vector<float>& samples) {
    if (samples.empty() || !ui_->enableEqCheckBox->isChecked()) {
        return;
    }

    try {
        configureTestEq(currentEnabledSettings());
        if (test_eq_ == nullptr) {
            return;
        }

        if (test_eq_buffer_.size() != samples.size()) {
            test_eq_buffer_ = MakeBuffer<float>(samples.size());
        }
        BufferRef<float> output(test_eq_buffer_);
        if (!test_eq_->Process(samples.data(), samples.size(), output) || output.empty()) {
            return;
        }
        samples.assign(output.begin(), output.end());
    }
    catch (...) {
        resetTestEq();
    }
}

void EqualizerView::applySetting(const QString& name, const EqSettings& settings) {
    current_name_ = name;
    current_settings_ = settings;

    if (current_settings_.bands.empty()) {
        current_settings_.SetDefault();
    }

    for (auto& band : current_settings_.bands) {
        if (band.type == EQFilterTypes::FT_UNKNOWN) {
            band.type = EQFilterTypes::FT_ALL_PEAKING_EQ;
        }
        if (band.frequency <= 0) {
            band.frequency = 1000;
        }
        if (band.Q <= 0) {
            band.Q = kDefaultQ;
        }
        band.band_width = 0;
    }

    if (ui_->eqPresetComboBox->findText(name) < 0) {
        ui_->eqPresetComboBox->addItem(name);
    }

    {
        const QSignalBlocker blocker(ui_->eqPresetComboBox);
        ui_->eqPresetComboBox->setCurrentText(name);
    }

    rebuildBandRows();
    updatePreampControl();
    updateGraph();
}

void EqualizerView::rebuildBandRows() {
    ClearLayout(ui_->bandsLayout);
    band_controls_.clear();

    ui_->bandsLayout->setColumnMinimumWidth(0, 46);
    ui_->bandsLayout->setColumnMinimumWidth(1, 260);
    ui_->bandsLayout->setColumnMinimumWidth(2, 140);
    ui_->bandsLayout->setColumnMinimumWidth(3, 110);
    ui_->bandsLayout->setColumnMinimumWidth(4, 170);
    ui_->bandsLayout->setColumnMinimumWidth(5, 22);
    ui_->bandsLayout->setColumnStretch(1, 3);
    ui_->bandsLayout->setColumnStretch(2, 1);
    ui_->bandsLayout->setColumnStretch(3, 1);
    ui_->bandsLayout->setColumnStretch(4, 1);
    ui_->bandsLayout->setVerticalSpacing(4);

    ui_->bandsLayout->addWidget(MakeHeaderLabel(ui_->bandsScrollAreaWidget, QStringLiteral("On"), Qt::AlignCenter), 0, 0);
    ui_->bandsLayout->addWidget(MakeHeaderLabel(ui_->bandsScrollAreaWidget, QStringLiteral("Curve"), Qt::AlignLeft | Qt::AlignVCenter), 0, 1);
    ui_->bandsLayout->addWidget(MakeHeaderLabel(ui_->bandsScrollAreaWidget, QStringLiteral("Gain"), Qt::AlignCenter), 0, 2);
    ui_->bandsLayout->addWidget(MakeHeaderLabel(ui_->bandsScrollAreaWidget, QStringLiteral("Q"), Qt::AlignCenter), 0, 3);
    ui_->bandsLayout->addWidget(MakeHeaderLabel(ui_->bandsScrollAreaWidget, QStringLiteral("Frequency"), Qt::AlignCenter), 0, 4);

    auto font = qTheme.monoFont();
    font.setPointSize(qTheme.fontSize(8));

    for (auto band = size_t{ 0 }; band < current_settings_.bands.size(); ++band) {
        const auto row = static_cast<int32_t>(band + 1);
        auto* enabled = new QCheckBox(this);
        enabled->setChecked(true);
        enabled->setFixedSize(36, kBandControlHeight);

        auto* curve = new QComboBox(this);
        AddCurveItems(curve);
        SetCurveValue(curve, current_settings_.bands[band].type);
        curve->setFixedHeight(kBandControlHeight);

        auto* gain = new QDoubleSpinBox(this);
        gain->setRange(kEQMinDb, kEQMaxDb);
        gain->setDecimals(1);
        gain->setSingleStep(0.5);
        gain->setSuffix(QStringLiteral(" dB"));
        gain->setAlignment(Qt::AlignCenter);
        gain->setValue(DisplayGain(current_settings_.bands[band].gain, effectivePreamp()));
        gain->setFixedHeight(kBandControlHeight);

        auto* q = new QDoubleSpinBox(this);
        q->setRange(0.10, 10.00);
        q->setDecimals(2);
        q->setSingleStep(0.05);
        q->setAlignment(Qt::AlignCenter);
        q->setValue(current_settings_.bands[band].Q);
        q->setFixedHeight(kBandControlHeight);

        auto* frequency = new QSpinBox(this);
        frequency->setRange(static_cast<int32_t>(kMinFrequency), 24000);
        frequency->setSingleStep(10);
        frequency->setSuffix(QStringLiteral(" Hz"));
        frequency->setAlignment(Qt::AlignCenter);
        frequency->setValue(static_cast<int32_t>(current_settings_.bands[band].frequency));
        frequency->setFixedHeight(kBandControlHeight);

        auto* color = new QLabel(this);
        color->setFixedSize(12, 12);
        color->setStyleSheet(BandColorStyle(kBandColors[band % kBandColors.size()]));

        ui_->bandsLayout->addWidget(enabled, row, 0, Qt::AlignCenter);
        ui_->bandsLayout->addWidget(curve, row, 1);
        ui_->bandsLayout->addWidget(gain, row, 2);
        ui_->bandsLayout->addWidget(q, row, 3);
        ui_->bandsLayout->addWidget(frequency, row, 4);
        ui_->bandsLayout->addWidget(color, row, 5, Qt::AlignCenter);

        frequency->setFont(font);
        gain->setFont(font);
        q->setFont(font);
        curve->setFont(font);
        band_controls_.push_back({ enabled, curve, gain, q, frequency, color });

        (void)QObject::connect(enabled, &QCheckBox::stateChanged, [this, curve, gain, q, frequency](auto value) {
            const auto is_enabled = value == Qt::CheckState::Checked;
            curve->setEnabled(is_enabled);
            gain->setEnabled(is_enabled);
            q->setEnabled(is_enabled);
            frequency->setEnabled(is_enabled);
            updateGraph();
            });
        (void)QObject::connect(curve, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this, band](auto) {
            updateBandFromUi(band);
            });
        (void)QObject::connect(gain, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this, band](auto) {
            updateBandFromUi(band);
            });
        (void)QObject::connect(q, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this, band](auto) {
            updateBandFromUi(band);
            });
        (void)QObject::connect(frequency, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this, band](auto) {
            updateBandFromUi(band);
            });
    }

    const auto band_rows = static_cast<int32_t>(current_settings_.bands.size());
    const auto spacing = std::max(ui_->bandsLayout->verticalSpacing(), 0);
    const auto content_height = kHeaderRowHeight
        + band_rows * kBandControlHeight
        + band_rows * spacing
        + 2;
    ui_->bandsScrollAreaWidget->setMinimumHeight(content_height);
    ui_->bandsScrollAreaWidget->setMaximumHeight(QWIDGETSIZE_MAX);
    ui_->bandsScrollArea->setMinimumHeight(std::min(content_height, kBandScrollAreaMaxHeight));
    ui_->bandsScrollArea->setMaximumHeight(kBandScrollAreaMaxHeight);
    updateGeometry();
}

void EqualizerView::updateBandFromUi(size_t band) {
    if (band >= current_settings_.bands.size() || band >= band_controls_.size()) {
        return;
    }

    const auto& controls = band_controls_[band];
    auto& setting = current_settings_.bands[band];
    setting.type = static_cast<EQFilterTypes>(controls.curve->currentData().toInt());
    setting.gain = BandGainFromDisplay(controls.gain->value(), effectivePreamp());
    setting.Q = static_cast<float>(controls.q->value());
    setting.frequency = static_cast<float>(controls.frequency->value());
    setting.band_width = 0;

    emit bandValueChanged(static_cast<int32_t>(band), setting.gain, setting.Q);
    updateGraph();
}

void EqualizerView::updateBandFromGraph(size_t band, float frequency, float gain) {
    if (band >= current_settings_.bands.size() || band >= band_controls_.size()) {
        return;
    }

    auto& setting = current_settings_.bands[band];
    setting.frequency = frequency;
    setting.gain = gain;
    setting.band_width = 0;

    const auto& controls = band_controls_[band];
    {
        const QSignalBlocker frequency_blocker(controls.frequency);
        controls.frequency->setValue(static_cast<int32_t>(std::round(frequency)));
    }
    {
        const QSignalBlocker gain_blocker(controls.gain);
        controls.gain->setValue(DisplayGain(gain, effectivePreamp()));
    }

    emit bandValueChanged(static_cast<int32_t>(band), setting.gain, setting.Q);
}

void EqualizerView::updateGraph() {
    std::vector<bool> enabled;
    enabled.reserve(band_controls_.size());
    for (const auto& controls : band_controls_) {
        enabled.push_back(controls.enabled->isChecked());
    }
    auto settings = current_settings_;
    settings.preamp = effectivePreamp();
    graph_->setSettings(settings, enabled);
}

void EqualizerView::updatePreampControl() {
    if (preamp_slider_ == nullptr || preamp_value_label_ == nullptr) {
        return;
    }

    const QSignalBlocker blocker(preamp_slider_);
    preamp_slider_->setValue(static_cast<int32_t>(std::round(current_settings_.preamp * kPreampScale)));
    preamp_value_label_->setText(FormatPreamp(current_settings_.preamp));
    updateBandGainControls();
}

void EqualizerView::updateBandGainControls() {
    const auto preamp = effectivePreamp();
    for (auto band = size_t{ 0 }; band < current_settings_.bands.size() && band < band_controls_.size(); ++band) {
        const QSignalBlocker blocker(band_controls_[band].gain);
        band_controls_[band].gain->setValue(DisplayGain(current_settings_.bands[band].gain, preamp));
    }
}

float EqualizerView::effectivePreamp() const {
    if (preamp_enabled_checkbox_ != nullptr && !preamp_enabled_checkbox_->isChecked()) {
        return 0;
    }
    return current_settings_.preamp;
}

void EqualizerView::saveCurrentSetting() {
    AppEQSettings app_settings;
    app_settings.name = current_name_;
    app_settings.settings = currentEnabledSettings();
    qAppSettings.setEqSettings(app_settings);
    qAppSettings.save();
}

EqSettings EqualizerView::currentEnabledSettings() const {
    auto settings = current_settings_;
    settings.preamp = effectivePreamp();
    settings.bands.clear();
    settings.bands.reserve(current_settings_.bands.size());
    for (auto band = size_t{ 0 }; band < current_settings_.bands.size(); ++band) {
        if (band < band_controls_.size() && !band_controls_[band].enabled->isChecked()) {
            continue;
        }
        settings.bands.push_back(current_settings_.bands[band]);
    }
    return settings;
}
