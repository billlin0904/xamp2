//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>

enum TagIOMode {
	TAG_IO_READ_MODE,
    TAG_IO_WRITE_MODE,
};

class XAMP_WIDGET_SHARED_EXPORT TagIO final {
public:
    TagIO();

    XAMP_DISABLE_COPY_AND_MOVE(TagIO)

	void Open(const Path& path, TagIOMode mode = TAG_IO_WRITE_MODE);

    static TrackInfo getTrackInfo(const Path& path);

    void writeArtist(const QString& artist);

    void writeAlbum(const QString& album);

    void writeTitle(const QString& title);

    void writeTrack(uint32_t track);

    void writeGenre(const QString& genre);

    void writeComment(const QString& comment);

    void writeYear(uint32_t year);

    QPixmap embeddedCover() const;

    bool embeddedCover(QPixmap& image, size_t& image_size) const;

    void removeEmbeddedCover();

    void writeEmbeddedCover(const QPixmap& image);

    XAMP_NO_DISCARD bool canWriteEmbeddedCover() const;

private:
    ScopedPtr<IMetadataReader> reader_;
    ScopedPtr<IMetadataWriter> writer_;
};