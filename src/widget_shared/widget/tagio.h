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

    static TrackInfo GetTrackInfo(const Path& path);

    void WriteArtist(const Path& path, const QString& artist);

    void WriteAlbum(const Path& path, const QString& album);

    void WriteTitle(const Path& path, const QString& title);

    void WriteTrack(const Path& path, uint32_t track);

    void WriteGenre(const Path& path, const QString& genre);

    void WriteComment(const Path& path, const QString& comment);

    void WriteYear(const Path& path, uint32_t year);

    QPixmap GetEmbeddedCover(const TrackInfo& track_info) const;

    QPixmap GetEmbeddedCover(const Path& file_path) const;

    bool GetEmbeddedCover(const Path& file_path, QPixmap& image, size_t& image_size) const;

    void RemoveEmbeddedCover(const Path& file_path);

    void WriteEmbeddedCover(const Path& file_path, const QPixmap& image);

    [[nodiscard]] bool CanWriteEmbeddedCover(const Path& path) const;

private:
    AlignPtr<IMetadataReader> reader_;
    AlignPtr<IMetadataWriter> writer_;
};