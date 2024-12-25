//=====================================================================================================================
// Copyright (c) 2018-2024 xamp project. All rights reserved.
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

struct XAMP_WIDGET_SHARED_EXPORT EncodeJob {
    uint32_t bit_rate{0};
    QString job_id;
    QString codec_id;
    PlayListEntity file;
};

Q_DECLARE_METATYPE(EncodeJob);

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

    QList<EncodeJob> addJobs(const QString &encode_name, const QList<PlayListEntity>& files);

public slots:
    void onUpdateProgress(const QString &job_id, int new_progress);

	void onJobError(const QString& job_id, const QString& message);
private:
    void updateProgressItem(QTreeWidgetItem* item, int new_progress);

    void setupUI();

    QTreeWidget* tree_;
    ProgressBarDelegate* progress_delegate_;
    QMap<QString, QTreeWidgetItem*> job_items_;
};