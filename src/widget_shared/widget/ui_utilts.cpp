#include <widget/ui_utilts.h>

#include <QDesktopWidget>
#include <QProgressBar>
#include <QCoreApplication>
#include <QApplication>
#include <QFileDialog>
#include <QRandomGenerator>
#include <QTime>

#include <stream/soxresampler.h>
#include <stream/r8brainresampler.h>
#include <stream/idspmanager.h>

#include <widget/appsettingnames.h>
#include <widget/xprogressdialog.h>
#include <widget/processindicator.h>
#include <widget/xmainwindow.h>
#include <widget/str_utilts.h>

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
#if 0
    if (playback_format.bit_rate > 0) {
        result += qTEXT(" | ") + FormatBitRate(playback_format.bit_rate);
    }
#endif
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
    QWidget* host = nullptr;

    if (!host)
        host = widget->parentWidget();

    if (host) {
        auto host_rect = host->geometry();
        widget->move(host_rect.center() - widget->rect().center());
    }
    else {
        QRect screenGeometry = QApplication::desktop()->screenGeometry();
        int x = (screenGeometry.width() - widget->width()) / 2;
        int y = (screenGeometry.height() - widget->height()) / 2;
        widget->move(x, y);
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
    const auto roll_off_level = static_cast<SoxrRollOff>(settings[kSoxrRollOffLevel].toInt());

    auto converter = MakeAlign<IAudioProcessor, SoxrSampleRateConverter>();
    auto* soxr_sample_rate_converter = dynamic_cast<SoxrSampleRateConverter*>(converter.get());
    soxr_sample_rate_converter->SetQuality(quality);
    soxr_sample_rate_converter->SetStopBand(stop_band);
    soxr_sample_rate_converter->SetPassBand(pass_band);
    soxr_sample_rate_converter->SetPhase(phase);
    soxr_sample_rate_converter->SetRollOff(roll_off_level);

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

QString GetExistingDirectory(QWidget* parent,
    const QString& caption,
    const QString& directory,
    const QString& filter) {
    QFileDialog dialog(parent, caption, directory, filter);
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setViewMode(QFileDialog::Detail);
    dialog.setOption(QFileDialog::ShowDirsOnly);
    QUrl selected_url;
    if (dialog.exec() == QDialog::Accepted) {
        selected_url = dialog.selectedUrls().value(0);
        if (selected_url.isLocalFile() || selected_url.isEmpty()) {
            return selected_url.toLocalFile();
        }            
    }
    return selected_url.toString();
}

static QString PickRandomProperty(const QMap<QString, QMap<QString, QString>>& obj) {
    int count = 0;
    QString result;
    QMap<QString, QMap<QString, QString>>::const_iterator it = obj.constBegin();
    while (it != obj.constEnd()) {
        if (QRandomGenerator::global()->generateDouble() < 1.0 / ++count)
            result = it.key();
        ++it;
    }
    return result;
}

static QString PickRandomProperty(const QMap<QString, QString>& obj) {
    int count = 0;
    QString result;
    auto it = obj.constBegin();
    while (it != obj.constEnd()) {
        if (QRandomGenerator::global()->generateDouble() < 1.0 / ++count)
            result = it.key();
        ++it;
    }
    return result;
}

QColor GenerateRandomColor() {
    static const QMap<QString, QMap<QString, QString>> colors = {
        {"red", {
            {"50", "#ffebee"},
            {"100", "#ffcdd2"},
            {"200", "#ef9a9a"},
            {"300", "#e57373"},
            {"400", "#ef5350"},
            {"500", "#f44336"},
            {"600", "#e53935"},
            {"700", "#d32f2f"},
            {"800", "#c62828"},
            {"900", "#b71c1c"},
            {"hex", "#f44336"},
            {"a100", "#ff8a80"},
            {"a200", "#ff5252"},
            {"a400", "#ff1744"},
            {"a700", "#d50000"}
        }},
        { "pink", {
        //{"50", "#fce4ec"},
        {"100" , "#f8bbd0"},
        {"200" , "#f48fb1"},
        {"300" , "#f06292"},
        {"400" , "#ec407a"},
        {"500" , "#e91e63"},
        {"600" , "#d81b60"},
        {"700" , "#c2185b"},
        {"800" , "#ad1457"},
        {"900" , "#880e4f"},
        {"hex" , "#e91e63"},
        {"a100" , "#ff80ab"},
        {"a200" , "#ff4081"},
        {"a400" , "#f50057"},
        {"a700" , "#c51162"},
      }},
      { "purple", {
		{"50", "#f3e5f5"},
		{"100" , "#e1bee7"},
		{"200" , "#ce93d8"},
		{"300" , "#ba68c8"},
		{"400" , "#ab47bc"},
		{"500" , "#9c27b0"},
		{"600" , "#8e24aa"},
		{"700" , "#7b1fa2"},
		{"800" , "#6a1b9a"},
		{"900" , "#4a148c"},
		{"hex" , "#9c27b0"},
		{"a100" , "#ea80fc"},
		{"a200" , "#e040fb"},
		{"a400" , "#d500f9"},
		{"a700" , "#aa00ff"},
	  }},
      { "deep-purple", {
		{"50", "#ede7f6"},
		{"100" , "#d1c4e9"},
		{"200" , "#b39ddb"},
		{"300" , "#9575cd"},
		{"400" , "#7e57c2"},
		{"500" , "#673ab7"},
		{"600" , "#5e35b1"},
		{"700" , "#512da8"},
		{"800" , "#4527a0"},
		{"900" , "#311b92"},
		{"hex" , "#673ab7"},
		{"a100" , "#b388ff"},
		{"a200" , "#7c4dff"},
		{"a400" , "#651fff"},
		{"a700" , "#6200ea"},
	  }},
      { "indigo", {
		{"50", "#e8eaf6"},
		{"100" , "#c5cae9"},
		{"200" , "#9fa8da"},
		{"300" , "#7986cb"},
		{"400" , "#5c6bc0"},
		{"500" , "#3f51b5"},
		{"600" , "#3949ab"},
		{"700" , "#303f9f"},
		{"800" , "#283593"},
		{"900" , "#1a237e"},
		{"hex" , "#3f51b5"},
		{"a100" , "#8c9eff"},
		{"a200" , "#536dfe"},
        }}
    };

    QString colorKey = PickRandomProperty(colors);
    QMap<QString, QString> colorList = colors.value(colorKey);
    QString newColorKey = PickRandomProperty(colorList);
    QString newColor = colorList.value(newColorKey);
    return QColor(newColor);
}

void Delay(int32_t seconds) {
    QTime dieTime = QTime::currentTime().addSecs(seconds);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}