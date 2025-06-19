#include <widget/util/ui_util.h>

#include <QApplication>
#include <QDirIterator>
#include <QFileDialog>
#include <QRandomGenerator>
#include <QScreen>
#include <QTime>
#include <QAbstractItemView>
#include <QHeaderView>
#include <QScrollBar>

#include <stream/soxresampler.h>
#include <stream/r8brainresampler.h>
#include <stream/srcresampler.h>
#include <stream/idspmanager.h>
#include <stream/filestream.h>

#include <widget/appsettings.h>
#include <widget/appsettingnames.h>
#include <widget/xprogressdialog.h>
#include <widget/processindicator.h>
#include <widget/xmainwindow.h>
#include <widget/util/str_util.h>
#include <widget/widget_shared.h>

#include <xampplayer.h>
#include <version.h>

#include <widget/playlistentity.h>
#include <widget/playlisttablemodel.h>
#include <widget/jsonsettings.h>

namespace {
    void saveLastOpenFolderPath(const QString& file_name) {
        if (QFileInfo(file_name).isDir()) {
            const QDir current_dir;
            qAppSettings.setValue(kAppSettingLastOpenFolderPath, current_dir.absoluteFilePath(file_name));
        }
        else {
            QDir current_dir(file_name);
            if (current_dir.cdUp()) {
                qAppSettings.setValue(kAppSettingLastOpenFolderPath, current_dir.path());
            } else {
                qAppSettings.setValue(kAppSettingLastOpenFolderPath, qAppSettings.myMusicFolderPath());
            }
        }
    }
}

QString formatSampleRate(const AudioFormat& format) {
    return formatSampleRate(format.GetSampleRate());
}

QString format2String(const PlaybackFormat& playback_format, const QString& file_ext) {
    auto format = playback_format.file_format;

    auto ext = file_ext;
    ext = ext.remove("."_str).toUpper();

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
        dsd_speed_format = "DSD"_str + QString::number(playback_format.dsd_speed);
        dsd_speed_format = "("_str + dsd_speed_format + ") | "_str;
        bits = 1;
    }

    QString output_format_str;
    QString dsd_mode;

    switch (playback_format.dsd_mode) {
    case DsdModes::DSD_MODE_PCM:
    case DsdModes::DSD_MODE_DSD2PCM:
        output_format_str = formatSampleRate(playback_format.file_format);
        if (playback_format.file_format.GetSampleRate() != playback_format.output_format.GetSampleRate()) {
            output_format_str += "/"_str + formatSampleRate(playback_format.output_format);
        }
        break;
    case DsdModes::DSD_MODE_NATIVE:
        dsd_mode = "Native DSD"_str;
        output_format_str = formatDsdSampleRate(playback_format.dsd_speed);
        break;
    case DsdModes::DSD_MODE_DOP:
        dsd_mode = "DOP"_str;
        output_format_str = formatDsdSampleRate(playback_format.dsd_speed);
        break;
    default: 
        break;
    }

    const auto bit_format = QString::number(bits) + "bit"_str;

    auto result = ext
        + " | "_str
        + dsd_speed_format
        + output_format_str
        + " | "_str
        + bit_format;

    if (!dsd_mode.isEmpty()) {
        result += " | "_str + dsd_mode;
    }

    if (playback_format.bit_rate > 0) {
        result += " | "_str + formatBitRate(playback_format.bit_rate);
    }

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

ScopedPtr<IAudioProcessor> makeR8BrainSampleRateConverter() {
    return MakeAlign<IAudioProcessor, R8brainSampleRateConverter>();
}

ScopedPtr<IAudioProcessor> makeSrcSampleRateConverter() {
    return MakeAlign<IAudioProcessor, SrcSampleRateConverter>();
}

ScopedPtr<IAudioProcessor> makeSoxrSampleRateConverter(uint32_t sample_rate) {
	const auto setting_name = qAppSettings.valueAsString(kAppSettingSoxrSettingName);
	QMap<QString, QVariant> soxr_settings = qJsonSettings.valueAs(kSoxr).toMap()[setting_name].toMap();
	soxr_settings[kResampleSampleRate] = sample_rate;
	return makeSoxrSampleRateConverter(soxr_settings);
}

ScopedPtr<IAudioProcessor> makeSoxrSampleRateConverter(const QVariantMap& settings) {
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

ScopedPtr<IAudioProcessor> makeSampleRateConverter(uint32_t sample_rate) {
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

IXMainWindow* getMainWindow() {
    IXMainWindow* main_window = nullptr;
    Q_FOREACH(auto* w, QApplication::topLevelWidgets()) {
        main_window = dynamic_cast<IXMainWindow*>(w);
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
        QString exts("("_str);
        const auto file_exts = GetSupportFileExtensions();
        for (const auto& file_ext : file_exts) {
            exts += "*"_str + QString::fromStdString(file_ext);
            exts += " "_str;
        }
        exts += ")"_str;
        file_extension = exts;
    }
    return file_extension;
}

QString getExistingDirectory(QWidget* parent, const QString &title, const std::function<void(const QString&)>& action) {
    auto last_open_folder = qAppSettings.valueAsString(kAppSettingLastOpenFolderPath);
    if (last_open_folder.isEmpty()) {
        last_open_folder = qAppSettings.myMusicFolderPath();
    }
    
    const auto dir_name = QFileDialog::getExistingDirectory(parent,
        title,
        last_open_folder,
        QFileDialog::ShowDirsOnly);
    if (dir_name.isNull()) {
        return dir_name;
    }
    saveLastOpenFolderPath(dir_name);
    if (action) {
        action(dir_name);
    }
    return dir_name;
}

void getOpenMusicFileName(QWidget* parent, const QString& title, const QString& files, std::function<void(const QString&)>&& action) {
    return getOpenFileName(parent,
        std::move(action),
        title,
        qAppSettings.myMusicFolderPath(),
        files + getFileDialogFileExtensions());
}

void getSaveFileName(QWidget* parent, 
    std::function<void(const QString&)>&& action,
    const QString& caption,
    const QString& dir,
    const QString& filter) {
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

QSharedPointer<XMessageBox> makeMessageBox(const QString& title,
    const QString& message, 
    QWidget* parent) {
    if (!parent) {
        parent = getMainWindow();
    }
    if (parent != nullptr) {
        parent->setFocus();
    }
    auto* message_box = new XMessageBox(title, message, parent);
    message_box->setIcon(qTheme.fontIcon(Glyphs::ICON_MESSAGE_BOX_INFORMATION));
	return QSharedPointer<XMessageBox>(message_box);
}

const QStringList& getTrackInfoFileNameFilter() {
    struct StaticGetFileNameFilter {
        StaticGetFileNameFilter() {
            for (auto& file_ext : GetSupportFileExtensions()) {
                name_filter << qFormat("*%1").arg(QString::fromStdString(file_ext));
            }
            name_filter << "*.cue"_str;
            //name_filter << "*.zip"_str;
        }
        QStringList name_filter;
    };
    return SharedSingleton<StaticGetFileNameFilter>::GetInstance().name_filter;
}

bool isSupportFileExtension(const QString& file_ext) {
    return GetSupportFileExtensions().contains(file_ext.toStdString());
}

size_t getFileCount(const QString& dir, const QStringList& file_name_filters) {
    QFileInfo info(dir);
    if (info.isFile()) {
        return 1;
    }

    QDirIterator itr(dir, file_name_filters, QDir::NoDotAndDotDot | QDir::Files, QDirIterator::Subdirectories);
    size_t file_count = 0;
    while (itr.hasNext()) {
        QFileInfo file_info(itr.next());
        if (file_info.isFile()) {
            ++file_count;
        }
    }
    return file_count;
}

QList<QModelIndex> getVisibleIndexes(const QAbstractItemView* list_view, int32_t column) {
    QList<QModelIndex> index_found;

    // Visible region of the viewport
    const auto region = list_view->viewport()->visibleRegion();

    // Function to check if index is visible
    auto is_index_visible = [&](const QModelIndex& index) {
        auto index_rect = list_view->visualRect(index);
        return region.contains(index_rect) || region.intersects(index_rect);
        };

    // Get the index of the first visible item
    auto first_index = list_view->indexAt(QPoint(0, 0));

    if (first_index.isValid()) {
        auto next_index = first_index;

        // Traverse items until index isn't visible
        while (is_index_visible(next_index)) {
            index_found.append(next_index);
            next_index = next_index.sibling(next_index.row() + 1, column);
        }
    }

    return index_found;
}

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

QString applicationPath() {
    return QDir::currentPath();
}

void setTabViewStyle(QTableView* table_view) {
    table_view->setStyleSheet(qFormat(R"(
	QTableView {
		background-color: transparent;
        border: 1px solid rgba(255, 255, 255, 10);
		border-radius: 4px;
	}

	QTableView::item:selected {
		background-color: rgba(255, 255, 255, 10);
	}
    )"));

    table_view->horizontalHeader()->setFixedHeight(30);
    table_view->horizontalHeader()->setStyleSheet(qFormat(R"(	
	QHeaderView::section {
		background-color: transparent;
		border-bottom: 1px solid rgba(255, 255, 255, 15);
	}

	QHeaderView::section::horizontal {
		padding-left: 4px;
		padding-right: 4px;
		font-size: 9pt;
	}

	QHeaderView::section::horizontal::first, QHeaderView::section::horizontal::only-one {
		border-left: 0px;
	}
    )"));

    auto f = table_view->font();
    f.setPointSize(qTheme.defaultFontSize());
    table_view->setFont(f);

    table_view->setUpdatesEnabled(true);
    table_view->setAcceptDrops(true);
    table_view->setDragEnabled(true);
    table_view->setShowGrid(false);
    table_view->setMouseTracking(true);
    table_view->setAlternatingRowColors(true);

    table_view->setDragDropMode(QAbstractItemView::InternalMove);
    table_view->setFrameShape(QAbstractItemView::NoFrame);
    table_view->setFocusPolicy(Qt::NoFocus);

    table_view->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    table_view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    table_view->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table_view->setSelectionBehavior(QAbstractItemView::SelectRows);

    table_view->viewport()->setAttribute(Qt::WA_StaticContents);

    table_view->verticalHeader()->setVisible(false);
    table_view->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    table_view->horizontalScrollBar()->setDisabled(true);

    table_view->horizontalHeader()->setVisible(true);
    table_view->horizontalHeader()->setHighlightSections(false);
    table_view->horizontalHeader()->setStretchLastSection(false);
    table_view->horizontalHeader()->setDefaultAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    table_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table_view->verticalScrollBar()->setStyleSheet(
        "QScrollBar:vertical { width: 6px; }"_str);

    table_view->verticalHeader()->setSectionsMovable(false);
    table_view->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
}

void showMeMessage(const QString& message) {
    if (qAppSettings.dontShowMeAgain(message)) {
        auto [button, checked] = XMessageBox::showCheckBoxInformation(
            message,
            qApp->tr("Ok, and don't show again."),
            kApplicationTitle,
            false);
        if (checked) {
            qAppSettings.addDontShowMeAgain(message);
        }
    }
}