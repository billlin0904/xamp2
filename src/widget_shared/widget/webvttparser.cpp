#include <fstream>
#include <locale>
#include <regex>

#include <base/str_utilts.h>
#include <widget/widget_shared.h>
#include <widget/webvttparser.h>

namespace {
    std::chrono::milliseconds parseTimecode(const std::wstring& timecode) {
        int hours, minutes, seconds, milliseconds;
        std::wistringstream ss(timecode);
        wchar_t discard;
        ss >> hours >> discard >> minutes >> discard >> seconds >> discard >> milliseconds;
        return std::chrono::hours(hours)
    	+ std::chrono::minutes(minutes)
    	+ std::chrono::seconds(seconds)
    	+ std::chrono::milliseconds(milliseconds);
    }
}

WebVTTParser::WebVTTParser() = default;

bool WebVTTParser::parse(std::wistream& file) {
    lyrics_.clear();

    enum ParseState {
	    PARSE_INDEX,
        PARSE_TIMESTAMP,
        PARSE_TEXT
    };

    std::wstring line;
    ParseState next_state{ PARSE_INDEX };

    auto wtoi = [](const auto str) {
        std::wstringstream istr(str);
        int val = 0;
        istr >> val;
        return val;
    };

    LyricEntry entry;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (line.find(L"WEBVTT") != std::wstring::npos) continue;

        switch (next_state) {
        case ParseState::PARSE_INDEX:
            entry.index = wtoi(line.c_str()) - 1;
            next_state = ParseState::PARSE_TIMESTAMP;
            break;
        case ParseState::PARSE_TIMESTAMP:
            if (std::regex_match(line,
                std::wregex(L"\\d{2}:\\d{2}:\\d{2}\\.\\d{3} --> \\d{2}:\\d{2}:\\d{2}\\.\\d{3}"))) {
                entry.start_time = parseTimecode(line.substr(0, 12));
                entry.end_time = parseTimecode(line.substr(17, 12));
                entry.timestamp = entry.start_time;
            }
            next_state = ParseState::PARSE_TEXT;
            break;
        case ParseState::PARSE_TEXT:
            entry.lrc = line;
            while (std::getline(file, line) && !line.empty()) {
                entry.lrc += L"\n" + line;
            }
            next_state = ParseState::PARSE_INDEX;
            lyrics_.push_back(entry);
            break;
        }
    }
    if (lyrics_.empty()) {
        return false;
    }
    return true;
}

bool WebVTTParser::parseFile(const std::wstring& file_path) {
    try {
        auto wide_str = String::ToStdWString(ReadFileToUtf8String(file_path));
        std::wstringstream file(wide_str);
        return parse(file);
    }
    catch (...) {
		return false;
    }
}

LyricEntry WebVTTParser::lineAt(int32_t index) const {
    return lyrics_[index];
}

int32_t WebVTTParser::getSize() const {
    return static_cast<int32_t>(lyrics_.size());
}

void WebVTTParser::clear() {
    lyrics_.clear();
}

void WebVTTParser::addLrc(const LyricEntry& lrc) {
    lyrics_.push_back(lrc);
}

const LyricEntry& WebVTTParser::getLyrics(const std::chrono::milliseconds& time) const noexcept {
    static LyricEntry last_subtitle;
    if (time < lyrics_.at(0).end_time) {
        return lyrics_.at(0);
    }
    for (const auto& subtitle : lyrics_) {
        if (time >= subtitle.start_time && time <= subtitle.end_time) {
            last_subtitle = subtitle;
            return subtitle;
        }
    }
    return last_subtitle;
}

std::vector<LyricEntry>::iterator WebVTTParser::end() {
    return lyrics_.end();
}

std::vector<LyricEntry>::iterator WebVTTParser::begin() {
    return lyrics_.begin();
}

std::vector<LyricEntry>::const_iterator WebVTTParser::cend() const {
    return lyrics_.cend();
}

std::vector<LyricEntry>::const_iterator WebVTTParser::cbegin() const {
    return lyrics_.cbegin();
}

bool WebVTTParser::hasTranslation() const {
    return false;
}
