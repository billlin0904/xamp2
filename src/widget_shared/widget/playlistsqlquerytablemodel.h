//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QSqlQueryModel>
#include <QVector>

class PlayListSqlQueryTableModel final : public QSqlQueryModel {
public:
    explicit PlayListSqlQueryTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void setRichAlbumHeaderEnabled(bool enabled);

    [[nodiscard]] bool isRichAlbumHeaderEnabled() const;

    void refreshRichAlbumHeaderRows();

private:
    struct RichDisplayRow {
        int source_row{ -1 };
        bool is_album_header{ false };
        int track_count{ 0 };
        double duration{ 0 };
    };

    void rebuildRichAlbumHeaderRows();

    [[nodiscard]] QModelIndex sourceIndex(const QModelIndex& index) const;

    bool rich_album_header_enabled_{ false };
    QVector<RichDisplayRow> rich_display_rows_;
};


