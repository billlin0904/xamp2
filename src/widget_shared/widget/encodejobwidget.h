//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QWidget>
#include <QMap>
#include <QTreeWidget>
#include <QStyledItemDelegate>
#include <QProgressBar>

#include <widget/widget_shared_global.h>
#include <widget/playlistentity.h>

enum EncodeType {
    ENCODE_ALAC,
    ENCODE_AAC,
    ENCODE_PCM,
};

struct XAMP_WIDGET_SHARED_EXPORT EncodeJob {
    EncodeType type{ ENCODE_AAC };
    uint32_t bit_rate{0};
    QString job_id;
    QString codec_id;
    PlayListEntity file;
};

Q_DECLARE_METATYPE(EncodeJob);

class QStandardItem;
class QStandardItemModel;

class XAMP_WIDGET_SHARED_EXPORT ProgressBarDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit ProgressBarDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter,
        const QStyleOptionViewItem& option,
        const QModelIndex& index) const override;
};

class XAMP_WIDGET_SHARED_EXPORT EncodeJobWidget : public QWidget {
    Q_OBJECT
public:
    explicit EncodeJobWidget(QWidget* parent = nullptr);

    QList<EncodeJob> addJobs(int32_t encode_type, const QList<PlayListEntity>& files);

public slots:
    void onUpdateProgress(const QString &job_id, int new_progress);

	void onJobError(const QString& job_id, const QString& message);
private:
    void updateProgressItem(const QPair<QStandardItem*, QStandardItem*> &item, int new_progress);

    void setupUI();

    QTreeView* tree_;
    ProgressBarDelegate* progress_delegate_;
	QStandardItemModel* model_;
    QMap<QString, QPair<QStandardItem*, QStandardItem*>> job_items_;
};