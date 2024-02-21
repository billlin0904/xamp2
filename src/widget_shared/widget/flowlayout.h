//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QLayout>
#include <QRect>
#include <QSize>
#include <QWidget>

class FlowLayout : public QLayout {
public:
    explicit FlowLayout(QWidget* parent = nullptr);

    virtual ~FlowLayout() override;

    void addItem(QLayoutItem* item) override;

    int count() const override;

    QLayoutItem* itemAt(int index) const override;

    QLayoutItem* takeAt(int index) override;

    Qt::Orientations expandingDirections() const override;

    bool hasHeightForWidth() const override;

    int heightForWidth(int width) const override;

    void setGeometry(const QRect& rect) override;

    QSize sizeHint() const override;

    QSize minimumSize() const override;

    void setVerticalSpacing(int spacing);

    int verticalSpacing() const;

    void setHorizontalSpacing(int spacing);

    int horizontalSpacing() const;

private:
    int doLayout(const QRect& rect, bool move) const;

    int vertical_spacing_;
    int horizontal_spacing_;
    QList<QLayoutItem*> items_;
};
