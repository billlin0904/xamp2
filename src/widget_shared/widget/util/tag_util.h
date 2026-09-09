//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstddef>

#include <QPixmap>

#include <metadata/imetadatareader.h>
#include <metadata/imetadatawriter.h>
#include <widget/widget_shared_global.h>
#include <widget/widget_shared.h>

namespace tag_util {

XAMP_WIDGET_SHARED_API std::optional<QImage> readEmbeddedCoverImage(IMetadataReader& reader);

XAMP_WIDGET_SHARED_API QPixmap readEmbeddedCover(IMetadataReader& reader);

XAMP_WIDGET_SHARED_API bool readEmbeddedCover(IMetadataReader& reader, QPixmap& image, size_t& image_size);

XAMP_WIDGET_SHARED_API void writeEmbeddedCover(IMetadataWriter& writer, const QPixmap& image);

}
