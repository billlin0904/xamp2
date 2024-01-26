#include <widget/ui_utilts.h>

#include <QApplication>
#include <QDirIterator>
#include <QFileDialog>
#include <QRandomGenerator>
#include <QScreen>
#include <QTime>

#include <stream/soxresampler.h>
#include <stream/r8brainresampler.h>
#include <stream/srcresampler.h>
#include <stream/idspmanager.h>

#include <widget/appsettings.h>
#include <widget/appsettingnames.h>
#include <widget/xprogressdialog.h>
#include <widget/processindicator.h>
#include <widget/xmainwindow.h>
#include <widget/str_utilts.h>

#include <version.h>

namespace {
    void saveLastOpenFolderPath(const QString& file_name) {
        if (QFileInfo(file_name).isDir()) {
            const QDir current_dir;
            qAppSettings.setValue(kAppSettingLastOpenFolderPath, current_dir.absoluteFilePath(file_name));
        }
        else {
            qAppSettings.setValue(kAppSettingLastOpenFolderPath, qAppSettings.myMusicFolderPath());
        }
    }
}

QString formatSampleRate(const AudioFormat& format) {
    return formatSampleRate(format.GetSampleRate());
}

QString format2String(const PlaybackFormat& playback_format, const QString& file_ext) {
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
        output_format_str = formatSampleRate(playback_format.file_format);
        if (playback_format.file_format.GetSampleRate() != playback_format.output_format.GetSampleRate()) {
            output_format_str += qTEXT("/") + formatSampleRate(playback_format.output_format);
        }
        break;
    case DsdModes::DSD_MODE_NATIVE:
        dsd_mode = qTEXT("Native DSD");
        output_format_str = formatDsdSampleRate(playback_format.dsd_speed);
        break;
    case DsdModes::DSD_MODE_DOP:
        dsd_mode = qTEXT("DOP");
        output_format_str = formatDsdSampleRate(playback_format.dsd_speed);
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

QSharedPointer<ProcessIndicator> makeProcessIndicator(QWidget* widget) {
    return {new ProcessIndicator(widget), &QObject::deleteLater};
}

QSharedPointer<XProgressDialog> makeProgressDialog(QString const& title,
    QString const& text, 
    QString const& cancel,
    QWidget* parent) {
    if (!parent) {
        parent = getMainWindow();
    }
    if (parent != nullptr) {
        parent->setFocus();
    }
    auto dialog = QSharedPointer<XProgressDialog>::create(text, cancel, 0, 100, parent);
    dialog->setTitle(title);
    return dialog;
}

void centerParent(QWidget* widget) {
    QWidget* host = nullptr;

    if (!host)
        host = widget->parentWidget();

    if (host) {
        auto host_rect = host->geometry();
        widget->move(host_rect.center() - widget->rect().center());
    }
    else {
	    const auto screen_geometry = QGuiApplication::primaryScreen()->geometry();
        const auto x = (screen_geometry.width() - widget->width()) / 2;
        const auto y = (screen_geometry.height() - widget->height()) / 2;
        widget->move(x, y);
    }
    widget->raise();
}

void centerDesktop(QWidget* widget) {
	const auto screen_geometry = QGuiApplication::primaryScreen()->geometry();
    const auto x = (screen_geometry.width() - widget->width()) / 2;
    const auto y = (screen_geometry.height() - widget->height()) / 2;
    widget->move(x, y);
}

void moveToTopWidget(QWidget* source_widget, const QWidget* target_widget) {
    auto center_pos = target_widget->mapToGlobal(target_widget->rect().center());
    const auto sz = source_widget->size();
    center_pos.setX(center_pos.x() - sz.width() / 2);
    center_pos.setY(center_pos.y() - 15 - sz.height());
    center_pos = source_widget->mapFromGlobal(center_pos);
    center_pos = source_widget->mapToParent(center_pos);
    source_widget->move(center_pos);
}

PlayerOrder getNextOrder(PlayerOrder cur) noexcept {
    auto next = static_cast<int32_t>(cur) + 1;
    auto max = static_cast<int32_t>(PlayerOrder::PLAYER_ORDER_MAX);
    return static_cast<PlayerOrder>(next % max);
}

AlignPtr<IAudioProcessor> makeR8BrainSampleRateConverter() {
    return MakeAlign<IAudioProcessor, R8brainSampleRateConverter>();
}

AlignPtr<IAudioProcessor> makeSrcSampleRateConverter() {
    return MakeAlign<IAudioProcessor, SrcSampleRateConverter>();
}

AlignPtr<IAudioProcessor> makeSoxrSampleRateConverter(const QVariantMap& settings) {
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

AlignPtr<IAudioProcessor> makeSampleRateConverter(uint32_t sample_rate) {
    QMap<QString, QVariant> soxr_settings;

    soxr_settings[kResampleSampleRate] = sample_rate;
    soxr_settings[kSoxrQuality] = static_cast<int32_t>(SoxrQuality::SINC_UHQ);
    soxr_settings[kSoxrPhase] = 46;
    soxr_settings[kSoxrStopBand] = 100;
    soxr_settings[kSoxrPassBand] = 96;
    soxr_settings[kSoxrRollOffLevel] = static_cast<int32_t>(SoxrRollOff::ROLLOFF_NONE);
    return makeSoxrSampleRateConverter(soxr_settings);
}

PlaybackFormat getPlaybackFormat(IAudioPlayer* player) {
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

XMainWindow* getMainWindow() {
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

QString getFileDialogFileExtensions() {
    static QString file_extension;
    if (file_extension.isEmpty()) {
        QString exts(qTEXT("("));
        auto file_exts = GetSupportFileExtensions();
        //file_exts.insert(".zip");
        for (const auto& file_ext : file_exts) {
            exts += qTEXT("*") + QString::fromStdString(file_ext);
            exts += qTEXT(" ");
        }
        exts += qTEXT(")");
        file_extension = exts;
    }
    return file_extension;
}

QString getExistingDirectory(QWidget* parent) {
    auto last_open_folder =qAppSettings.valueAsString(kAppSettingLastOpenFolderPath);
    if (last_open_folder.isEmpty()) {
        last_open_folder = qAppSettings.myMusicFolderPath();
    }
    
    const auto dir_name = QFileDialog::getExistingDirectory(parent,
        qTR("Select a directory"),
        last_open_folder,
        QFileDialog::ShowDirsOnly);

    saveLastOpenFolderPath(dir_name);

    return dir_name;
}

void getOpenMusicFileName(QWidget* parent, std::function<void(const QString&)>&& action) {
    return getOpenFileName(parent,
        std::move(action),
        qTR("Open file"),
        qAppSettings.myMusicFolderPath(),
        qTR("Music Files ") + getFileDialogFileExtensions());
}

void getSaveFileName(QWidget* parent, 
    std::function<void(const QString&)>&& action,
    const QString& caption,
    const QString& dir,
    const QString& filter) {
    const auto last_dir = qAppSettings.valueAsString(kAppSettingLastOpenFolderPath);
    const auto file_name = QFileDialog::getSaveFileName(parent,
        caption,
        dir,
        filter,
        nullptr);

    if (file_name.isNull()) {
        return;
    }

    saveLastOpenFolderPath(file_name);

    action(file_name);
}

void getOpenFileName(QWidget* parent, 
    std::function<void(const QString&)> && action,
    const QString& caption,
    const QString& dir, 
    const QString& filter) {
    const auto last_dir = qAppSettings.valueAsString(kAppSettingLastOpenFolderPath);
    const auto file_name = QFileDialog::getOpenFileName(parent,
        caption,
        dir,
        filter,
        nullptr);

    if (file_name.isNull()) {
        return;
    }

    saveLastOpenFolderPath(file_name);
    
    action(file_name);    
}

void delay(int32_t seconds) {
	const auto die_time = QTime::currentTime().addSecs(seconds);
    while (QTime::currentTime() < die_time)
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 100);
}

const QStringList& getTrackInfoFileNameFilter() {
    struct StaticGetFileNameFilter {
        StaticGetFileNameFilter() {
            for (auto& file_ext : GetSupportFileExtensions()) {
                name_filter << qSTR("*%1").arg(QString::fromStdString(file_ext));
            }
        }
        QStringList name_filter;
    };
    return SharedSingleton<StaticGetFileNameFilter>::GetInstance().name_filter;
}

size_t getFileCount(const QString& dir, const QStringList& file_name_filters) {
    if (QFileInfo(dir).isFile()) {
        return 1;
    }

    QDirIterator itr(dir, file_name_filters, QDir::NoDotAndDotDot | QDir::Files, QDirIterator::Subdirectories);
    size_t file_count = 0;
    while (itr.hasNext()) {
        if (QFileInfo(itr.next()).isFile()) {
            ++file_count;
        }
    }
    return file_count;
}