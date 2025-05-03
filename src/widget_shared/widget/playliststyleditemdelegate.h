//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/widget_shared.h>
#include <widget/databasecoverid.h>

#include <QStyledItemDelegate>

class XAMP_WIDGET_SHARED_EXPORT PlaylistStyledItemDelegate final : public QStyledItemDelegate {
	Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    static constexpr auto kPlayingStateIconSize = 8;
    static constexpr QSize kIconSize = QSize(kPlayingStateIconSize, kPlayingStateIconSize);
    static constexpr auto kImageCacheSize = 24;

    explicit PlaylistStyledItemDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

signals:
    void findAlbumCover(const DatabaseCoverId& id) const;
};