//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstdint>
#include <string>
#include <chrono>
#include <regex>
#include <vector>

#include <base/base.h>

struct LyricEntry {
    int32_t index{0};
    std::chrono::milliseconds timestamp{0};
    std::wstring lrc;
};

class LrcParser {
public:
	LrcParser();

    XAMP_DISABLE_COPY(LrcParser)

    bool parseFile(const std::wstring &file_path);

    bool parse(std::wistream &istr);

    void clear();

    std::wstring maxLengthLrc() const;

    std::vector<LyricEntry>::iterator end();

    std::vector<LyricEntry>::iterator begin();

    LyricEntry last() const;

    LyricEntry lineAt(int32_t index) const;

    std::chrono::milliseconds getDuration() const;

    void addLrc(const LyricEntry &lrc);

    int32_t getInterval() const;

    const LyricEntry& getLyrics(const std::chrono::milliseconds &time) const noexcept;

    int32_t getSize() const;

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
