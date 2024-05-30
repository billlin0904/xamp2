//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/widget_shared.h>

#include <QStyledItemDelegate>

class PlayListStyledItemDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    static constexpr auto kPlayingStateIconSize = 8;
    static constexpr QSize kIconSize = QSize(kPlayingStateIconSize, kPlayingStateIconSize);
    static constexpr auto kImageCacheSize = 24;

    explicit PlayListStyledItemDelegate(QObject* parent = nullptr);

    QIcon visibleCovers(const QString& cover_id) const;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};