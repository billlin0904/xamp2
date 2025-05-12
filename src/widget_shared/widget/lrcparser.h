//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstdint>
#include <string>
#include <chrono>
#include <regex>
#include <vector>

#include <base/base.h>
#include <base/furigana.h>
#include <widget/widget_shared.h>
#include <widget/ilrrcparser.h>

class LrcParser : public ILrcParser {
public:
	LrcParser();

    virtual ~LrcParser();

    XAMP_DISABLE_COPY(LrcParser)

    bool parseFile(const std::wstring &file_path) override;

    bool parse(std::wistream &istr) override;

    void clear() override;

    std::wstring maxLengthLrc() const;

    std::vector<LyricEntry>::iterator end() override;

    std::vector<LyricEntry>::iterator begin() override;

    std::vector<LyricEntry>::const_iterator cend() const override;

    std::vector<LyricEntry>::const_iterator cbegin() const override;

    LyricEntry last() const override;

    LyricEntry lineAt(int32_t index) const override;

    std::chrono::milliseconds getDuration() const;

    void addLrc(const LyricEntry &lrc) override;

    int32_t getInterval() const;

    const LyricEntry& getLyrics(const std::chrono::milliseconds &time) const noexcept override;

    int32_t size() const override;

	bool hasTranslation() const override;

	bool isKaraoke() const override;
private:
    void parseLrc(std::wstring const & line);

    void parseMultiLrc(std::wstring const & line);

    bool parseStream(std::wistream &istr);

	int32_t offset_;
    std::wstring title_;
    std::wstring artist_;
    std::wstring album_;
    std::wstring by_;
    std::vector<LyricEntry> lyrics_;
    std::wregex pattern_;
};
