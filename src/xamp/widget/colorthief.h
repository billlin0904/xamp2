//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QImage>

std::vector<QColor> GetPalette(const QString& image_file_path, int32_t color_count = 10, int32_t quality = 10);
std::vector<QColor> GetPalette(const QImage& image, int32_t color_count = 10, int32_t quality = 10);