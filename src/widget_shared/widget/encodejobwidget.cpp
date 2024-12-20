#include <QApplication>
#include <QHeaderView>
#include <QVBoxLayout>

#include <widget/widget_shared.h>
#include <widget/encodejobwidget.h>

enum {
	ENCODE_LIST_JOB_TITLE = 0,
    ENCODE_LIST_FILE_NAME,
    ENCODE_LIST_FORMAT,
    ENCODE_LIST_PROGRESS,
    ENCODE_LIST_STATE,
};

ProgressBarDelegate::ProgressBarDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {
}

void ProgressBarDelegate::paint(QPainter* painter,
    const QStyleOptionViewItem& option,
    const QModelIndex& index) const {
    QVariant value = index.data(Qt::UserRole + 1);
    // 如果此item沒有任何進度資料 (可能是群組標題那一行), value將不是有效的整數
    if (!value.isValid()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    const int progress = value.toInt();
    auto rect = option.rect;

    QStyleOptionProgressBar progress_bar_option;
    progress_bar_option.direction = Qt::LayoutDirection::LeftToRight;
    progress_bar_option.rect = rect;
    progress_bar_option.state = QStyle::State_Enabled | QStyle::State_Horizontal;
    progress_bar_option.progress = progress;
    progress_bar_option.maximum = 100;
    progress_bar_option.minimum = 0;
    progress_bar_option.text = QString::number(progress) + "%"_str;
    progress_bar_option.textVisible = true;

    QApplication::style()->drawControl(QStyle::CE_ProgressBar, &progress_bar_option, painter);
}

EncodeJobWidget::EncodeJobWidget(QWidget* parent)
    : QWidget(parent)
	, tree_(new QTreeWidget(this))
	, progress_delegate_(new ProgressBarDelegate(this)) {
    setupUI();
}

QList<EncodeJob> EncodeJobWidget::addJobs(const QString& encode_name, const QList<PlayListEntity>& files) {
    constexpr auto k24Bit441KhzBitRate = 2116.8;

    QList<EncodeJob> jobs;
    auto* job_item = new QTreeWidgetItem(tree_);
    job_item->setText(0, qFormat(tr("%1 Files")).arg(files.size()));

    for (const auto& file : files) {
        auto* child = new QTreeWidgetItem(job_item);
        child->setText(ENCODE_LIST_FILE_NAME, file.file_name);

        EncodeJob job;
        job.job_id = QString::fromStdString(generateUuid().toStdString());
        job.file = file;

        if (encode_name == "ALAC"_str) {
            child->setText(ENCODE_LIST_FORMAT, qFormat(tr("%1 | %2 | %3 bit"))
                .arg(encode_name)
                .arg(formatSampleRate(file.sample_rate))
                .arg(file.bit_rate < k24Bit441KhzBitRate ? 16 : 24));
            job.codec_id = "alac"_str;
        }
        else {
            child->setText(ENCODE_LIST_FORMAT, qFormat(tr("%1 | %2 | %3 bit | 256kbps"))
                .arg(encode_name)
                .arg(formatSampleRate(file.sample_rate))
                .arg(file.bit_rate < k24Bit441KhzBitRate ? 16 : 24));
			job.bit_rate = 256000;
			job.codec_id = "aac"_str;
        }

        child->setData(ENCODE_LIST_PROGRESS, Qt::UserRole + 1, 0);
        jobs.append(job);
        job_items_.insert(job.job_id, child);
        child->setText(ENCODE_LIST_STATE, tr("Pending"));
    }

    tree_->header()->resizeSections(QHeaderView::ResizeToContents);
    int currentWidth = tree_->header()->sectionSize(3);
    if (currentWidth < 100) {
        tree_->header()->resizeSection(ENCODE_LIST_PROGRESS, 200);
    }
    tree_->header()->resizeSection(ENCODE_LIST_FILE_NAME, 500);
    tree_->header()->resizeSection(ENCODE_LIST_FORMAT, 200);

    job_item->setExpanded(true);
    return jobs;
}

void EncodeJobWidget::updateProgressItem(QTreeWidgetItem* item, int new_progress) {
    if (!item) return;
    if (new_progress != 100) {
        item->setText(ENCODE_LIST_STATE, tr("Processing"));
    } else {
        item->setText(ENCODE_LIST_STATE, tr("Completed"));
    }
    item->setData(ENCODE_LIST_PROGRESS, Qt::UserRole + 1, new_progress);
    tree_->viewport()->update();
}

void EncodeJobWidget::updateProgress(const QString& job_id, int new_progress) {
    auto itr = job_items_.find(job_id);
    if (itr != job_items_.end()) {
        updateProgressItem(itr.value(), new_progress);
    }
}

void EncodeJobWidget::setupUI() {
    QStringList headers;
    headers << tr("Job") << tr("File Name") << tr("File Format") << tr("Progress") << tr("State");
    tree_->setHeaderLabels(headers);
    tree_->setRootIsDecorated(true);
    tree_->setAllColumnsShowFocus(true);
    tree_->setItemDelegateForColumn(ENCODE_LIST_PROGRESS, progress_delegate_);
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(tree_);
    setLayout(layout);
}
