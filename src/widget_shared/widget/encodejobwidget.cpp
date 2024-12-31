#include <QApplication>
#include <QHeaderView>
#include <QStandardItemModel>
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
	, tree_(new QTreeView(this))
    , progress_delegate_(new ProgressBarDelegate(this))
	, model_(new QStandardItemModel(this)) {
    setupUI();
}

QList<EncodeJob> EncodeJobWidget::addJobs(const QString& encode_name, const QList<PlayListEntity>& files) {
    constexpr auto k24Bit441KhzBitRate = 2116.8;

    QList<EncodeJob> jobs;

    auto* job_item = new QStandardItem(qFormat("%1 Files").arg(files.size()));

    QList<QStandardItem*> top_row;
    top_row << job_item
        << new QStandardItem(QString())
        << new QStandardItem(QString())
        << new QStandardItem(QString())
        << new QStandardItem(QString());
    model_->appendRow(top_row);

    for (const auto& file : files) {
        auto child0 = new QStandardItem(QString()); // job
        auto child1 = new QStandardItem(file.file_name);   // file name
        auto child2 = new QStandardItem(); // format
        auto child3 = new QStandardItem(); // progress
        auto child4 = new QStandardItem("Pending"_str); // state

        QList<QStandardItem*> row_items;
        row_items << child0 << child1 << child2 << child3 << child4;

        EncodeJob job;
        job.job_id = QString::fromStdString(generateUuid().toStdString()); // or ...
        job.file = file;

        if (encode_name == "ALAC"_str) {
            child2->setText(qFormat("%1 | %2 | %3 bit")
                .arg(encode_name)
                .arg(formatSampleRate(file.sample_rate))
                .arg(file.bit_rate < k24Bit441KhzBitRate ? 16 : 24));
            job.codec_id = "alac"_str;
        }
        else {
            child2->setText(qFormat("%1 | %2 | %3 bit | 256kbps")
                .arg(encode_name)
                .arg(formatSampleRate(file.sample_rate))
                .arg(file.bit_rate < k24Bit441KhzBitRate ? 16 : 24));
            job.bit_rate = 256000;
            job.codec_id = "aac"_str;
        }

        child3->setData(0, Qt::UserRole + 1);
        job_item->appendRow(row_items);
        job_items_.insert(job.job_id, qMakePair(child3, child4));

        jobs.append(job);
    }

    tree_->expand(job_item->index());

    tree_->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    tree_->header()->setSectionResizeMode(ENCODE_LIST_PROGRESS, QHeaderView::Interactive);
    tree_->setColumnWidth(ENCODE_LIST_PROGRESS, 300);
    tree_->header()->resizeSection(ENCODE_LIST_FILE_NAME, 800);
    tree_->header()->resizeSection(ENCODE_LIST_FORMAT, 500);

    return jobs;
}

void EncodeJobWidget::updateProgressItem(const QPair<QStandardItem*, QStandardItem*>& item, int new_progress) {
    if (new_progress != 100) {
        item.second->setText(tr("Processing"));
    }
    else {
        item.second->setText(tr("Completed"));
    }

    item.first->setData(new_progress, Qt::UserRole + 1);

    if (tree_->viewport()) {
        tree_->viewport()->update();
    }
}

void EncodeJobWidget::onUpdateProgress(const QString& job_id, int new_progress) {
    auto itr = job_items_.find(job_id);
    if (itr != job_items_.end()) {
        updateProgressItem(itr.value(), new_progress);
    }
}

void EncodeJobWidget::onJobError(const QString& job_id, const QString& message) {
    auto itr = job_items_.find(job_id);
    if (itr != job_items_.end()) {
        itr.value().second->setText(message);
    }
}

void EncodeJobWidget::setupUI() {
    QStringList headers;
    headers << tr("Job") << tr("File Name") << tr("File Format")
        << tr("Progress") << tr("State");
    model_->setColumnCount(headers.size());
    model_->setHorizontalHeaderLabels(headers);

    tree_->setModel(model_);
    tree_->setRootIsDecorated(true);
    tree_->setAllColumnsShowFocus(true);

    tree_->setItemDelegateForColumn(ENCODE_LIST_PROGRESS, progress_delegate_);
 
    auto layout = new QVBoxLayout(this);
    layout->addWidget(tree_);
    setLayout(layout);

    setStyleSheet(qFormat(R"(
	QTreeView {
		background-color: transparent;
        border: 1px solid rgba(255, 255, 255, 10);
		border-radius: 4px;
	}

	QTreeView::item:selected {
		background-color: rgba(255, 255, 255, 10);
	}
    )"));

    tree_->header()->setFixedHeight(30);
    tree_->header()->setStyleSheet(qFormat(R"(	
	QHeaderView {
		background-color: transparent;
	}

	QHeaderView::section {
		background-color: transparent;
		border-bottom: 1px solid rgba(255, 255, 255, 15);
	}

	QHeaderView::section::horizontal {
		font-size: 9pt;
	}

	QHeaderView::section::horizontal::first, QHeaderView::section::horizontal::only-one {
		border-left: 0px;
	}
    )"));
}
