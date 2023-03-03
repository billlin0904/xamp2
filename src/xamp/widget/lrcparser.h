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

    bool ParseFile(const std::wstring &file_path);

    bool Parse(std::wistream &istr);

    void Clear();

    int32_t GetMaxLrcLength() const;

    std::wstring GetMaxLengthLrc() const;

    std::vector<LyricEntry>::iterator end();

    std::vector<LyricEntry>::iterator begin();

    LyricEntry Last() const;

    LyricEntry LineAt(int32_t index) const;

    std::chrono::milliseconds GetDuration() const;

    void AddLrc(const LyricEntry &lrc);

    int32_t GetInterval() const;

    const LyricEntry& GetLyrics(const std::chrono::milliseconds &time) const noexcept;

    int32_t GetSize() const;

private:
    void ParseLrc(std::wstring const & line);

    void ParseMultiLrc(std::wstring const & line);

    bool ParseStream(std::wistream &istr);

	int32_t offset_;
    std::wstring title_;
    std::wstring artist_;
    std::wstring album_;
    std::wstring by_;
    std::vector<LyricEntry> lyrics_;
    std::wregex pattern_;
};
