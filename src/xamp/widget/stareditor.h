//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QWidget>
#include <QMouseEvent>

#include <widget/starrating.h>

class StarEditor : public QWidget {
    Q_OBJECT
public:
    StarEditor(int row, QWidget* parent = nullptr);

    QSize sizeHint() const;

    void setStarRating(const StarRating& starRating) {
        rating_ = starRating;
    }

    const StarRating & starRating() const { 
        return rating_; 
    }

    int row() const {
        return row_;
    }

signals:
    void editingFinished();

protected:
    void paintEvent(QPaintEvent* event);

    void mouseMoveEvent(QMouseEvent* event);

    void mouseReleaseEvent(QMouseEvent* event);

private:
    int starAtPosition(int x) const;

    int row_;
    StarRating rating_;
};

