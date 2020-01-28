//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QListView>
#include <QSqlTableModel>
#include <QStyledItemDelegate>

class ArtistViewStyledDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit ArtistViewStyledDelegate(QObject* parent = nullptr);

protected:
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    QSize sizeHint(const QStyleOptionViewItem& o, const QModelIndex& idx) const override;
};

class ArtistView : public QListView {
    Q_OBJECT
public:
    ArtistView(QWidget *parent = nullptr);

public slots:
    void refreshOnece();

private:
    QSqlTableModel model_;
};
