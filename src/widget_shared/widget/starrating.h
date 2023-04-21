//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QMetaType>
#include <QPointF>
#include <QVector>
#include <QPainter>
#include <QPalette>

#include <widget/widget_shared_global.h>

class XAMP_WIDGET_SHARED_EXPORT StarRating {
public:
    enum EditMode { 
        Editable,
        ReadOnly
    };

    StarRating(int starCount = 1, int maxStarCount = 5);

    void paint(QPainter* painter, const QRect& rect, const QPalette& palette, EditMode mode) const;

    QSize sizeHint() const;

    int starCount() const { 
        return rating_count_;
    }

    int maxStarCount() const { 
        return max_rating_count_;
    }

    void setStarCount(int starCount) { 
        rating_count_ = starCount;
    }

    void setMaxStarCount(int maxStarCount) { 
        max_rating_count_ = maxStarCount;
    }

private:
    QPolygonF polygon_;
    QPolygonF diamond_;
    int32_t rating_count_;
    int32_t max_rating_count_;
};

Q_DECLARE_METATYPE(StarRating)