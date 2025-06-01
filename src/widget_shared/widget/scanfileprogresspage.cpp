#include <widget/scanfileprogresspage.h>
#include <widget/util/str_util.h>
#include <widget/util/ui_util.h>
#include <widget/xmainwindow.h>

#include <thememanager.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSpacerItem>

ScanFileProgressPage::ScanFileProgressPage(QWidget* parent)
    : QFrame(parent) {
    setObjectName("scanFileProgressPage"_str);
    setFrameStyle(QFrame::StyledPanel);

    progress_bar_ = new QProgressBar(this);
    progress_bar_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    progress_bar_->setFixedHeight(15);

    auto* close_button_ = new QPushButton(this);
    close_button_->setObjectName("scanFileProgressPageCloseButton"_str);
    close_button_->setCursor(Qt::PointingHandCursor);
    close_button_->setAttribute(Qt::WA_TranslucentBackground);
    close_button_->setFixedSize(qTheme.titleButtonIconSize());
    close_button_->setIconSize(qTheme.titleButtonIconSize());
    close_button_->setIcon(qTheme.fontIcon(Glyphs::ICON_CLOSE_WINDOW,
        qTheme.themeColor()));

    auto* button_spacer = new QSpacerItem(20,
        5,
        QSizePolicy::Expanding,
        QSizePolicy::Expanding);
    auto* hbox_layout = new QHBoxLayout();
    hbox_layout->setSpacing(0);
    hbox_layout->setContentsMargins(10, 0, 0, 0);
    hbox_layout->addSpacerItem(button_spacer);
    hbox_layout->addWidget(close_button_);

    message_text_label_ = new QLabel(this);
    message_text_label_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    message_text_label_->setObjectName("messageTextLabel"_str);
    message_text_label_->setOpenExternalLinks(false);
    message_text_label_->setFixedHeight(20);
    message_text_label_->setSizePolicy(QSizePolicy::Expanding,
        QSizePolicy::Fixed);
    message_text_label_->setStyleSheet("background: transparent;"_str);

    auto* layout_ = new QVBoxLayout(this);
    layout_->addLayout(hbox_layout);
    layout_->addWidget(message_text_label_);
    layout_->addWidget(progress_bar_);
    layout_->setSizeConstraint(QLayout::SetNoConstraint);
    layout_->setSpacing(5);
    layout_->setContentsMargins(5, 5, 5, 5);

    setStyleSheet(qFormat(
        R"(
           QFrame#scanFileProgressPage {
		        background-color: %1;
                border-top-left-radius: 4px;
				border-top-right-radius: 4px;
                padding: 5px;
           }
        )"
    ).arg(qTheme.linearGradientStyle()));

    (void)QObject::connect(close_button_, &QPushButton::clicked, [this]() {
        emit cancelRequested();
        });
}

void ScanFileProgressPage::onReadFileProgress(int32_t progress) {
    progress_bar_->setValue(progress);
    getMainWindow()->setTaskbarProgress(progress);
}

void ScanFileProgressPage::onReadCompleted() {
    hide();
    progress_bar_->setValue(0);
    getMainWindow()->resetTaskbarProgress();
}

void ScanFileProgressPage::onRemainingTimeEstimation(size_t total_work,
    size_t completed_work,
    int32_t secs) {
    auto message =
        qFormat("Remaining Time: %1 seconds, process file total: %2, completed: %3.")
        .arg(formatDuration(secs))
        .arg(total_work)
        .arg(completed_work);
    QFontMetrics metrics(font());
    message_text_label_->setText(metrics.elidedText(message, Qt::ElideRight, width()));
}

void ScanFileProgressPage::onFoundFileCount(size_t file_count) {
    auto message = qFormat("Total number of files %1").arg(file_count);
    QFontMetrics metrics(font());
    message_text_label_->setText(metrics.elidedText(message, Qt::ElideRight, width()));
}

void ScanFileProgressPage::onReadFilePath(const QString& file_path) {
    QFontMetrics metrics(font());
    auto message = metrics.elidedText(file_path, Qt::ElideRight, width() - 100);
    message_text_label_->setText(message);
}

void ScanFileProgressPage::onReadFileStart() {
    getMainWindow()->resetTaskbarProgress();
}