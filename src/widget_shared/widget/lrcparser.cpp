﻿#include <widget/lrcparser.h>
#include <fstream>

#include <base/fs.h>
#include <base/algorithm.h>
#include <base/str_utilts.h>
#include <base/logger.h>
#include <base/logger_impl.h>

#include <widget/widget_shared_global.h>
#include <widget/widget_shared.h>

namespace {
    std::chrono::milliseconds parseTime(const std::wstring& str) {
        auto hours = 0;
        auto minutes = 0;
        auto seconds = 0;
        auto milliseconds = 0;
        
        auto res = port_swscanf(str.c_str(), L"%u:%u.%u",
            &minutes,
            &seconds,
            &milliseconds);
        if (res == 3) {
            return std::chrono::minutes(minutes)
                + std::chrono::seconds(seconds)
                + std::chrono::milliseconds(milliseconds);
        }

        res = port_swscanf(str.c_str(), L"%u:%u:%u",
            &hours,
            &minutes,
            &seconds);
        if (res == 3) {
            return std::chrono::hours(hours)
                + std::chrono::minutes(minutes)
                + std::chrono::seconds(seconds);
        }
		throw std::invalid_argument("Invalid time format");
    }

    template <typename ReturnType = std::wstring>
    struct TagParser {
        static ReturnType parseIdTag(const std::wstring& tag_name, const std::wstring& line) {
            const auto tag_prefix = L"[" + tag_name + L":";

            auto pos = line.find(tag_prefix);
            if (pos != std::wstring::npos) {
                auto end = line.find(L"]", pos);
                auto start = pos + tag_prefix.length();
                return line.substr(start, end - start);
            }
            return L"";
        }
    };

    template <>
    struct TagParser<int> {
        static int parseIdTag(const std::wstring& tag_name, const std::wstring& line) {
            auto str = TagParser<>::parseIdTag(tag_name, line);
            if (str.empty()) {
                return 0;
            }
            return std::stoi(str);
        }
    };
}

LrcParser::LrcParser()
    : offset_(0)
	, pattern_(LR"(\[\d{2}:\d{2}(?::\d{2}(?:\.\d{2,3})?|\.\d{2,3})?\] ?[^\[]*)") {
}

bool LrcParser::parse(std::wistream &istr) {
    return parseStream(istr);
}

void LrcParser::clear() {
    lyrics_.clear();
}

bool LrcParser::parseFile(const std::wstring &file_path) {
    try {
        auto wide_str = String::ToStdWString(ReadFileToUtf8String(file_path));
        std::wstringstream file(wide_str);
        return parseStream(file);
    }
    catch (...) {
        return false;
    }   
}

void LrcParser::parseLrc(const std::wstring & line) {
	std::wsregex_iterator itr(line.begin(), line.end(), pattern_);

    for (; itr != std::wsregex_iterator(); ++itr) {
        auto match = *itr;
        auto res = match.str();		

        auto time = res;
        time.assign(time.c_str() + 1, time.find(']') - 1);

        LyricEntry lrc;
        lrc.timestamp = parseTime(time);
        lrc.lrc = res.assign(res.c_str() + res.find(']') + 1);
        lyrics_.push_back(lrc);
    }
}

void LrcParser::parseMultiLrc(std::wstring const & line) {
    std::vector<std::wstring> times;
    std::wstring time;

    auto temp = line;

    while (true) {
        auto first = temp.find(L"[");
        auto last = temp.find(L"]");
        if (first == std::string::npos || last == std::string::npos) {
            break;
        }
        time = temp.substr(first + 1, last - first - 1);
        times.push_back(time);
        temp.erase(first, last - first + 1);
    }

    auto lrc = temp;
    for (auto const &time : times) {
        LyricEntry entry;
        entry.timestamp = parseTime(time);
        entry.lrc = lrc;
        lyrics_.push_back(entry);
    }
}

bool LrcParser::parseStream(std::wistream &istr) {
	lyrics_.clear();

    bool start_read_lrc = false;

	constexpr wchar_t dlm = L'\n';

	std::wstring line;
    while (std::getline(istr, line, dlm)) {
		if (!start_read_lrc) {
			if (line.find(L"[ti") != std::wstring::npos) {
				title_ = TagParser<>::parseIdTag(L"ti", line);
				XAMP_LOG_DEBUG("title: {}", String::ToUtf8String(title_));
				continue;
			}
			if (line.find(L"[ar") != std::wstring::npos) {
				artist_ = TagParser<>::parseIdTag(L"ar", line);
				XAMP_LOG_DEBUG("artist: {}", String::ToUtf8String(artist_));
				continue;
			}
			if (line.find(L"[al") != std::wstring::npos) {
				album_ = TagParser<>::parseIdTag(L"al", line);
				XAMP_LOG_DEBUG("album: {}", String::ToUtf8String(album_));
				continue;
			}
			if (line.find(L"[offset") != std::wstring::npos) {
				offset_ = TagParser<int32_t>::parseIdTag(L"offset", line);
				XAMP_LOG_DEBUG("offset: {}", offset_);
				continue;
			}
		}		
		if (istr.eof()) {
			parseLrc(line);
			break;
		}
        auto count = std::count(line.begin(), line.end(), L']');
        if (count == 1) {
            parseLrc(line);
			start_read_lrc = true;
        } else {
            parseMultiLrc(line);
			start_read_lrc = true;
        }        
	}

	XAMP_LOG_DEBUG("Total lyrics: {}", lyrics_.size());

	if (lyrics_.empty()) {
		return false;
	}

    std::stable_sort(lyrics_.begin(), lyrics_.end(),
        [](const LyricEntry &a, const LyricEntry &b) {
		return a.timestamp < b.timestamp;
	});

	if (!lyrics_.empty()) {
		lyrics_.insert(lyrics_.begin(), LyricEntry());
	}

    for (size_t i = 0; i < lyrics_.size(); ++i) {
		lyrics_[i].index = i;
	}	

    return true;
}

std::wstring LrcParser::maxLengthLrc() const {
    const auto itr = std::max_element(lyrics_.begin(), lyrics_.end(),
        [](const LyricEntry& a, const LyricEntry& b) {
        return a.lrc.length() < b.lrc.length();
        });

    if (itr != lyrics_.end()) {
        return (*itr).lrc;
    }
    return L"";
}

std::vector<LyricEntry>::iterator LrcParser::end() {
    return lyrics_.end();
}

std::vector<LyricEntry>::iterator LrcParser::begin() {
    return lyrics_.begin();
}

std::vector<LyricEntry>::const_iterator LrcParser::cend() const {
    return lyrics_.cend();
}

std::vector<LyricEntry>::const_iterator LrcParser::cbegin() const {
    return lyrics_.cbegin();
}

LyricEntry LrcParser::last() const {
    return lyrics_.back();
}

LyricEntry LrcParser::lineAt(const int32_t index) const {
    return lyrics_[index];
}

std::chrono::milliseconds LrcParser::getDuration() const {
    return lyrics_.back().timestamp;
}

int32_t LrcParser::getInterval() const {
    return getDuration().count() / lyrics_.size();
}

void LrcParser::addLrc(const LyricEntry &lrc) {
    lyrics_.push_back(lrc);
}

const LyricEntry& LrcParser::getLyrics(const std::chrono::milliseconds &time) const noexcept {
    auto itr = BinarySearch(lyrics_.cbegin(),
        lyrics_.cend(), time, [](const LyricEntry &l, auto time) {
        return l.timestamp < time;
    });
    
    if (itr != lyrics_.cbegin()) {		
		if (itr == lyrics_.end()) {
			return lyrics_.back();
		}
	    // 不是倒數最後一個選前一個
	    itr -= 1;
	    return (*itr);
    }
    return *lyrics_.cbegin();
}

int32_t LrcParser::size() const {
    return static_cast<int32_t>(lyrics_.size());
}

bool LrcParser::hasTranslation() const {
    return false;
}

bool LrcParser::isKaraoke() const {
    return false;
}
