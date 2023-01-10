#include <QDesktopWidget>
#include <QProgressBar>
#include <qcoreapplication.h>
#include <QApplication>

#include <widget/xprogressdialog.h>
#include <widget/processindicator.h>
#include <widget/str_utilts.h>
#include <widget/ui_utilts.h>

QString sampleRate2String(const AudioFormat& format) {
    return sampleRate2String(format.GetSampleRate());
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
        output_format_str = sampleRate2String(playback_format.file_format);
        if (playback_format.file_format.GetSampleRate() != playback_format.output_format.GetSampleRate()) {
            output_format_str += qTEXT("/") + sampleRate2String(playback_format.output_format);
        }
        break;
    case DsdModes::DSD_MODE_NATIVE:
        dsd_mode = qTEXT("Native DSD");
        output_format_str = dsdSampleRate2String(playback_format.dsd_speed);
        break;
    case DsdModes::DSD_MODE_DOP:
        dsd_mode = qTEXT("DOP");
        output_format_str = dsdSampleRate2String(playback_format.dsd_speed);
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
        result += qTEXT(" |") + bitRate2String(playback_format.bitrate);
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
    auto desktop = QApplication::desktop();
    auto screen_num = desktop->screenNumber(QCursor::pos());
    QRect rect = desktop->screenGeometry(screen_num);
    widget->move(rect.center() - widget->rect().center());
}

void centerTarget(QWidget* source_widget, QWidget* target_widget) {
    auto center_pos = target_widget->mapToGlobal(target_widget->rect().center());
    const auto sz = source_widget->size();
    center_pos.setX(center_pos.x() - sz.width() / 2);
    center_pos.setY(center_pos.y() - 32 - sz.height());
    center_pos = source_widget->mapFromGlobal(center_pos);
    center_pos = source_widget->mapToParent(center_pos);
    source_widget->move(center_pos);
}