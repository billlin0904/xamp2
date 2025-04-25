//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <widget/widget_shared_global.h>

struct XAMP_WIDGET_SHARED_EXPORT LyricWord {
    int32_t extraParam{ 0 };
    std::chrono::milliseconds offset{ 0 };
    std::chrono::milliseconds length{ 0 };    
    std::wstring content;
};

struct XAMP_WIDGET_SHARED_EXPORT LyricEntry {
    int32_t index{ 0 };
    std::chrono::milliseconds timestamp{ 0 };
    std::chrono::milliseconds start_time{ 0 };
    std::chrono::milliseconds end_time{ 0 };
    // 未翻譯歌詞
    std::wstring lrc;
    // 翻譯後歌詞
    std::wstring tlrc;
	std::vector<LyricWord> words;
};

class XAMP_WIDGET_SHARED_EXPORT ILrcParser {
public:
	virtual ~ILrcParser() = default;

    virtual bool parseFile(const std::wstring& file_path) = 0;

    virtual bool parse(std::wistream& istr) = 0;

    virtual int32_t size() const = 0;

    virtual void clear() = 0;

    virtual LyricEntry lineAt(int32_t index) const = 0;

    virtual const LyricEntry& getLyrics(const std::chrono::milliseconds& time) const noexcept = 0;

    virtual std::vector<LyricEntry>::iterator end() = 0;

    virtual std::vector<LyricEntry>::iterator begin() = 0;

    virtual std::vector<LyricEntry>::const_iterator cend() const = 0;

    virtual std::vector<LyricEntry>::const_iterator cbegin() const = 0;

	virtual LyricEntry last() const = 0;

    virtual void addLrc(const LyricEntry& lrc) = 0;

    virtual bool hasTranslation() const = 0;

    virtual bool isKaraoke() const = 0;
protected:
	ILrcParser() = default;
};


