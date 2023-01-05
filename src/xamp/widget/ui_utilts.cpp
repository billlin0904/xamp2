#include <QDesktopWidget>
#include <QProgressBar>
#include <QCheckBox>
#include <QProgressDialog>

#include <version.h>
#include <widget/xdialog.h>
#include <widget/xmessagebox.h>
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
    return QSharedPointer<ProcessIndicator>(new ProcessIndicator(widget), &QObject::deleteLater);
}

QSharedPointer<QProgressDialog> makeProgressDialog(QString const& title, QString const& text, QString const& cancel) {
    auto* dialog = new QProgressDialog(text, cancel, 0, 100);
    dialog->setFont(qApp->font());
    dialog->setWindowTitle(title);
    dialog->setWindowModality(Qt::WindowModal);
    dialog->setFixedSize(QSize(1000, 100));
    dialog->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    dialog->setWindowFlags(dialog->windowFlags() & ~Qt::WindowContextHelpButtonHint);
    auto* progress_bar = new QProgressBar(dialog);
    progress_bar->setFont(QFont(qTEXT("FormatFont")));
    dialog->setBar(progress_bar);
    return QSharedPointer<QProgressDialog>(dialog);
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
