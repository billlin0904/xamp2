//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>

class TagIO final {
public:
    TagIO();

    XAMP_DISABLE_COPY_AND_MOVE(TagIO)

    static TrackInfo getTrackInfo(const Path& path);

    void writeArtist(const Path& path, const QString& artist);

    void writeAlbum(const Path& path, const QString& album);

    void writeTitle(const Path& path, const QString& title);

    void writeTrack(const Path& path, uint32_t track);

    void writeGenre(const Path& path, const QString& genre);

    void writeComment(const Path& path, const QString& comment);

    void writeYear(const Path& path, uint32_t year);

    QPixmap embeddedCover(const TrackInfo& track_info) const;

    QPixmap embeddedCover(const Path& file_path) const;

    bool embeddedCover(const Path& file_path, QPixmap& image, size_t& image_size) const;

    void removeEmbeddedCover(const Path& file_path);

    void writeEmbeddedCover(const Path& file_path, const QPixmap& image);

    [[nodiscard]] bool canWriteEmbeddedCover(const Path& path) const;

private:
    AlignPtr<IMetadataReader> reader_;
    AlignPtr<IMetadataWriter> writer_;
};