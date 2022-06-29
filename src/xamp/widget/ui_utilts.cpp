#include <QDesktopWidget>
#include <QProgressBar>
#include <QCheckBox>
#include <QProgressDialog>

#include <widget/xdialog.h>
#include <widget/str_utilts.h>
#include <widget/ui_utilts.h>
#include <version.h>

QString sampleRate2String(const AudioFormat& format) {
    return samplerate2String(format.GetSampleRate());
}

QString format2String(const PlaybackFormat& playback_format, const QString& file_ext) {
    auto format = playback_format.file_format;

    auto ext = file_ext;
    ext = ext.remove(Q_TEXT(".")).toUpper();

    auto precision = 1;
    auto is_mhz_sample_rate = false;
    if (format.GetSampleRate() / 1000 > 1000) {
        is_mhz_sample_rate = true;
    }
    else {
        precision = format.GetSampleRate() % 1000 == 0 ? 0 : 1;
    }

    auto bits = format.GetBitsPerSample();

    QString dsd_speed_format;
    if (playback_format.is_dsd_file
        && (playback_format.dsd_mode == DsdModes::DSD_MODE_NATIVE || playback_format.dsd_mode == DsdModes::DSD_MODE_DOP)) {
        dsd_speed_format = Q_TEXT("DSD") + QString::number(playback_format.dsd_speed);
        dsd_speed_format = Q_TEXT("(") + dsd_speed_format + Q_TEXT(") | ");
        bits = 1;
    }
    else {
        dsd_speed_format = Q_TEXT(" ");
    }

    QString output_format_str;
    QString dsd_mode;

    switch (playback_format.dsd_mode) {
    case DsdModes::DSD_MODE_PCM:
    case DsdModes::DSD_MODE_DSD2PCM:
        dsd_mode = Q_TEXT("PCM");
        output_format_str = sampleRate2String(playback_format.file_format);
        if (playback_format.file_format.GetSampleRate() != playback_format.output_format.GetSampleRate()) {
            output_format_str += Q_TEXT("/") + sampleRate2String(playback_format.output_format);
        }
        break;
    case DsdModes::DSD_MODE_NATIVE:
        dsd_mode = Q_TEXT("Native DSD");
        output_format_str = dsdSampleRate2String(playback_format.dsd_speed);
        break;
    case DsdModes::DSD_MODE_DOP:
        dsd_mode = Q_TEXT("DOP");
        output_format_str = dsdSampleRate2String(playback_format.dsd_speed);
        break;
    default: 
        break;
    }

    const auto bit_format = QString::number(bits) + Q_TEXT("bit");

    return ext
        + dsd_speed_format
        + bit_format
        + Q_TEXT("/")
        + output_format_str
        + Q_TEXT(" | ") + dsd_mode;
}

QSharedPointer<QProgressDialog> makeProgressDialog(QString const& title, QString const& text, QString const& cancel) {
    auto* dialog = new QProgressDialog(text, cancel, 0, 100);
    dialog->setFont(qApp->font());
    dialog->setWindowTitle(title);
    dialog->setWindowModality(Qt::WindowModal);
    dialog->setMinimumSize(QSize(1000, 100));
    dialog->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
    auto* progress_bar = new QProgressBar();
    progress_bar->setFont(QFont(Q_TEXT("FormatFont")));
    dialog->setBar(progress_bar);
    return QSharedPointer<QProgressDialog>(dialog);
}

std::tuple<bool, QMessageBox::StandardButton> showDontShowAgainDialog(bool show_agin) {
    bool is_show_agin = true;

    if (show_agin) {
        auto cb = new QCheckBox(qApp->tr("Don't show this again"));
        QMessageBox msgbox;
        msgbox.setWindowTitle(kAppTitle);
        msgbox.setText(qApp->tr("Hide XAMP to system tray?"));
        msgbox.setIcon(QMessageBox::Icon::Question);
        msgbox.addButton(QMessageBox::Ok);
        msgbox.addButton(QMessageBox::Cancel);
        msgbox.setDefaultButton(QMessageBox::Cancel);
        msgbox.setCheckBox(cb);

        (void)QObject::connect(cb, &QCheckBox::stateChanged, [&is_show_agin](int state) {
            if (static_cast<Qt::CheckState>(state) == Qt::CheckState::Checked) {
                is_show_agin = false;
            }
            });
        return { is_show_agin, static_cast<QMessageBox::StandardButton>(msgbox.exec()) };
    }
    return { is_show_agin, QMessageBox::Yes };
}

void centerParent(QWidget* dialog) {
    if (dialog->parent() && dialog->parent()->isWidgetType()) {
        dialog->move((dialog->parentWidget()->width() - dialog->width()) / 2,
            (dialog->parentWidget()->height() - dialog->height()) / 2);
    }
}

void centerDesktop(QWidget* widget) {
    auto desktop = QApplication::desktop();
    auto screen_num = desktop->screenNumber(QCursor::pos());
    QRect rect = desktop->screenGeometry(screen_num);
    widget->move(rect.center() - widget->rect().center());
}

