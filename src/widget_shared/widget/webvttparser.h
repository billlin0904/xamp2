//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <vector>

#include <base/base.h>
#include <widget/ilrrcparser.h>

class WebVTTParser : public ILrcParser {
public:
    WebVTTParser();

    XAMP_DISABLE_COPY(WebVTTParser)

	bool parse(std::wistream& istr) override;

	bool parseFile(const std::wstring& file_path) override;

    LyricEntry lineAt(int32_t index) const override;

    int32_t getSize() const override;

    void clear() override;

    void addLrc(const LyricEntry& lrc) override;

    const LyricEntry& getLyrics(const std::chrono::milliseconds& time) const noexcept override;

    std::vector<LyricEntry>::iterator end() override;

    std::vector<LyricEntry>::iterator begin() override;

    std::vector<LyricEntry>::const_iterator cend() const override;

    std::vector<LyricEntry>::const_iterator cbegin() const override;
private:
    std::vector<LyricEntry> lyrics_;
};