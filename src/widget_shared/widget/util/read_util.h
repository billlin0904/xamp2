//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/widget_shared.h>
#include <stream/ebur128scanner.h>

#include <QByteArray>

void readAll(Path const& file_path,
    std::function<bool(uint32_t)> const& progress,
    std::function<void(AudioFormat const&)> const& prepare,
    std::function<void(float const*, uint32_t)> const& dsp_process,
    uint64_t max_duration = (std::numeric_limits<uint64_t>::max)());

Ebur128Scanner readFileLoudness(const Path& file_path,
    const std::function<bool(uint32_t)>& progress);

QByteArray readChromaprint(const Path& file_path);

ScopedPtr<FileStream> makePcmFileStream(const Path& file_path);
