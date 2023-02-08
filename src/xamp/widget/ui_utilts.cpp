#include <QDesktopWidget>
#include <QProgressBar>
#include <QCoreApplication>
#include <QApplication>

#include <base/assert.h>
#include <stream/soxresampler.h>
#include <stream/r8brainresampler.h>
#include <stream/idspmanager.h>

#include <widget/appsettingnames.h>
#include <widget/xprogressdialog.h>
#include <widget/processindicator.h>
#include <widget/xmainwindow.h>
#include <widget/str_utilts.h>
#include <widget/ui_utilts.h>

QString FormatSampleRate(const AudioFormat& format) {
    return FormatSampleRate(format.GetSampleRate());
}

QString Format2String(const PlaybackFormat& playback_format, const QString& file_ext) {
    auto format = playback_format.file_format;

    auto ext = file_ext;
    ext = ext.remove(qTEXT(".")).toUpper();

    auto precision = 1;
    auto is_mhz_sample_rate = false;
    if (format.GetSampleRate() / 1000 > 1000) {
        is_mhz_sample_rate = true;
    }
    else {
        precision = format.GetSampleRate() % 1000 == 0 ? 0 : 1;
    }

    auto bits = (std::max)(format.GetBitsPerSample(), 16U);

    QString dsd_speed_format;
    if (playback_format.is_dsd_file
        && (playback_format.dsd_mode == DsdModes::DSD_MODE_NATIVE || playback_format.dsd_mode == DsdModes::DSD_MODE_DOP)) {
        dsd_speed_format = qTEXT("DSD") + QString::number(playback_format.dsd_speed);
        dsd_speed_format = qTEXT("(") + dsd_speed_format + qTEXT(") | ");
        bits = 1;
    }

    QString output_format_str;
    QString dsd_mode;

    switch (playback_format.dsd_mode) {
    case DsdModes::DSD_MODE_PCM:
    case DsdModes::DSD_MODE_DSD2PCM:
        output_format_str = FormatSampleRate(playback_format.file_format);
        if (playback_format.file_format.GetSampleRate() != playback_format.output_format.GetSampleRate()) {
            output_format_str += qTEXT("/") + FormatSampleRate(playback_format.output_format);
        }
        break;
    case DsdModes::DSD_MODE_NATIVE:
        dsd_mode = qTEXT("Native DSD");
        output_format_str = FormatDsdSampleRate(playback_format.dsd_speed);
        break;
    case DsdModes::DSD_MODE_DOP:
        dsd_mode = qTEXT("DOP");
        output_format_str = FormatDsdSampleRate(playback_format.dsd_speed);
        break;
    default: 
        break;
    }

    const auto bit_format = QString::number(bits) + qTEXT("bit");

    auto result = ext
        + qTEXT(" | ")
        + dsd_speed_format
        + output_format_str
        + qTEXT(" | ")
        + bit_format;

    if (!dsd_mode.isEmpty()) {
        result += qTEXT(" | ") + dsd_mode;
    }
    if (playback_format.bit_rate > 0) {
        result += qTEXT(" | ") + FormatBitRate(playback_format.bit_rate);
    }
    return result;
}

QSharedPointer<ProcessIndicator> MakeProcessIndicator(QWidget* widget) {
    return {new ProcessIndicator(widget), &QObject::deleteLater};
}

QSharedPointer<XProgressDialog> MakeProgressDialog(QString const& title,
    QString const& text, 
    QString const& cancel,
    QWidget* parent) {
    if (!parent) {
        parent = GetMainWindow();
    }
    if (parent != nullptr) {
        parent->setFocus();
    }
    auto* dialog = new XProgressDialog(text, cancel, 0, 100, parent);
    dialog->setFont(qApp->font());
    dialog->setWindowTitle(title);
    dialog->setWindowModality(Qt::ApplicationModal);
	dialog->show();
    return QSharedPointer<XProgressDialog>(dialog);
}

void CenterParent(QWidget* widget) {
    if (widget->parent() && widget->parent()->isWidgetType()) {
        widget->move((widget->parentWidget()->width() - widget->width()) / 2,
            (widget->parentWidget()->height() - widget->height()) / 2);
    } else {
        CenterDesktop(widget);
    }
}

void CenterDesktop(QWidget* widget) {
	const auto screen_geometry = QApplication::desktop()->screenGeometry();
    auto x = (screen_geometry.width() - widget->width()) / 2;
    auto y = (screen_geometry.height() - widget->height()) / 2;
    widget->move(x, y);
}

void MoveToTopWidget(QWidget* source_widget, const QWidget* target_widget) {
    auto center_pos = target_widget->mapToGlobal(target_widget->rect().center());
    const auto sz = source_widget->size();
    center_pos.setX(center_pos.x() - sz.width() / 2);
    center_pos.setY(center_pos.y() - 15 - sz.height());
    center_pos = source_widget->mapFromGlobal(center_pos);
    center_pos = source_widget->mapToParent(center_pos);
    source_widget->move(center_pos);
}

PlayerOrder GetNextOrder(PlayerOrder cur) noexcept {
    auto next = static_cast<int32_t>(cur) + 1;
    auto max = static_cast<int32_t>(PlayerOrder::PLAYER_ORDER_MAX);
    return static_cast<PlayerOrder>(next % max);
}

AlignPtr<IAudioProcessor> MakeR8BrainSampleRateConverter() {
    return MakeAlign<IAudioProcessor, R8brainSampleRateConverter>();
}

AlignPtr<IAudioProcessor> MakeSoxrSampleRateConverter(const QVariantMap& settings) {
    const auto quality = static_cast<SoxrQuality>(settings[kSoxrQuality].toInt());
    const auto stop_band = settings[kSoxrStopBand].toInt();
    const auto pass_band = settings[kSoxrPassBand].toInt();
    const auto phase = settings[kSoxrPhase].toInt();
    const auto enable_steep_filter = settings[kSoxrEnableSteepFilter].toBool();
    const auto roll_off_level = static_cast<SoxrRollOff>(settings[kSoxrRollOffLevel].toInt());

    auto converter = MakeAlign<IAudioProcessor, SoxrSampleRateConverter>();
    auto* soxr_sample_rate_converter = dynamic_cast<SoxrSampleRateConverter*>(converter.get());
    soxr_sample_rate_converter->SetQuality(quality);
    soxr_sample_rate_converter->SetStopBand(stop_band);
    soxr_sample_rate_converter->SetPassBand(pass_band);
    soxr_sample_rate_converter->SetPhase(phase);
    soxr_sample_rate_converter->SetRollOff(roll_off_level);
    soxr_sample_rate_converter->SetSteepFilter(enable_steep_filter);
    return converter;
}

PlaybackFormat GetPlaybackFormat(IAudioPlayer* player) {
    PlaybackFormat format;

    if (player->IsDsdFile()) {
        format.dsd_mode = player->GetDsdModes();
        format.dsd_speed = *player->GetDsdSpeed();
        format.is_dsd_file = true;
    }

    format.enable_sample_rate_convert = player->GetDspManager()->IsEnableSampleRateConverter();
    format.file_format = player->GetInputFormat();
    format.output_format = player->GetOutputFormat();
    return format;
}

XMainWindow* GetMainWindow() {
    XMainWindow* main_window = nullptr;
    Q_FOREACH(auto* w, QApplication::topLevelWidgets()) {
        main_window = dynamic_cast<XMainWindow*>(w);
        if (main_window != nullptr) {
            break;
        }
    }
    XAMP_ENSURES(main_window != nullptr);
    return main_window;
}
