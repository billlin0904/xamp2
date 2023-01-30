#include <QDesktopWidget>
#include <QProgressBar>
#include <QCoreApplication>
#include <QApplication>

#include <base/assert.h>

#include <widget/xprogressdialog.h>
#include <widget/processindicator.h>
#include <widget/xwindow.h>
#include <widget/str_utilts.h>
#include <widget/ui_utilts.h>

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
    if (playback_format.bitrate > 0) {
        result += qTEXT(" |") + formatBitRate(playback_format.bitrate);
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
        parent = qApp->activeWindow();
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

void centerParent(QWidget* widget) {
    if (widget->parent() && widget->parent()->isWidgetType()) {
        widget->move((widget->parentWidget()->width() - widget->width()) / 2,
            (widget->parentWidget()->height() - widget->height()) / 2);
    }
}

void centerDesktop(QWidget* widget) {
	const auto desktop = QApplication::desktop();
    const auto screen_num = desktop->screenNumber(QCursor::pos());
	const auto rect = desktop->screenGeometry(screen_num);
    widget->move(rect.center() - widget->rect().center());
}

void centerTarget(QWidget* source_widget, const QWidget* target_widget) {
    auto center_pos = target_widget->mapToGlobal(target_widget->rect().center());
    const auto sz = source_widget->size();
    center_pos.setX(center_pos.x() - sz.width() / 2);
    center_pos.setY(center_pos.y() - 15 - sz.height());
    center_pos = source_widget->mapFromGlobal(center_pos);
    center_pos = source_widget->mapToParent(center_pos);
    source_widget->move(center_pos);
}

XWindow* getMainWindow() {
    XWindow* main_window = nullptr;
    Q_FOREACH(auto* w, QApplication::topLevelWidgets()) {
        main_window = qobject_cast<XWindow*>(w);
        if (main_window != nullptr) {
            break;
        }
    }
    XAMP_ENSURES(main_window != nullptr);
    return main_window;
}
