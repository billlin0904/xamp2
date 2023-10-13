#include <widget/lrcparser.h>
#include <fstream>

#include <base/fs.h>
#include <base/algorithm.h>
#include <base/str_utilts.h>
#include <base/logger.h>
#include <base/logger_impl.h>

#include <widget/widget_shared.h>

namespace {
    std::chrono::milliseconds ParseTime(std::wstring const& str) {
        auto minutes = 0;
        auto seconds = 0;
        auto milliseconds = 0;
#ifdef Q_OS_WIN32
        // todo: Support format: %u:%u
        const auto res = swscanf_s(str.c_str(), L"%u:%u.%u",
            &minutes,
            &seconds,
            &milliseconds);
        if (res != 3) {
            throw std::exception("Malformed lrc file");
        }
#else
        const auto res = swscanf(str.c_str(), L"%u:%u.%u",
            &minutes,
            &seconds,
            &milliseconds);
        if (res != 3) {
            throw std::exception();
        }
#endif
        return std::chrono::minutes(minutes)
            + std::chrono::seconds(seconds)
            + std::chrono::milliseconds(milliseconds);
    }

    template <typename ReturnType = std::wstring>
    struct TagParser {
        static ReturnType ParseIdTag(const std::wstring& tag_name, const std::wstring& line) {
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
        static int ParseIdTag(const std::wstring& tag_name, const std::wstring& line) {
            auto str = TagParser<>::ParseIdTag(tag_name, line);
            if (str.empty()) {
                return 0;
            }
            return std::stoi(str);
        }
    };
}

LrcParser::LrcParser()
    : offset_(0)
	, pattern_(L"\\[\\d*:\\d{2}\\.\\d{1,3}\\][^\\[]*") {
}

bool LrcParser::Parse(std::wistream &istr) {
    return ParseStream(istr);
}

void LrcParser::Clear() {
    lyrics_.clear();
}

bool LrcParser::ParseFile(const std::wstring &file_path) {
    std::wifstream file;
    file.open(file_path);

    if (!file.is_open()) {
        return false;
    }

    ImbueFileFromBom(file);
    return ParseStream(file);
}

void LrcParser::ParseLrc(const std::wstring & line) {
	std::wsregex_iterator itr(line.begin(), line.end(), pattern_);

    for (; itr != std::wsregex_iterator(); ++itr) {
        auto match = *itr;
        auto res = match.str();		

        auto time = res;
        time.assign(time.c_str() + 1, time.find(']') - 1);

        LyricEntry lrc;
        lrc.timestamp = ParseTime(time);
        lrc.lrc = res.assign(res.c_str() + res.find(']') + 1);
        lyrics_.push_back(lrc);
    }
}

void LrcParser::ParseMultiLrc(std::wstring const & line) {
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
        entry.timestamp = ParseTime(time);
        entry.lrc = lrc;
        lyrics_.push_back(entry);
    }
}

bool LrcParser::ParseStream(std::wistream &istr) {
	lyrics_.clear();

    bool start_read_lrc = false;

    wchar_t dlm = L'\n';

	std::wstring line;
    while (std::getline(istr, line, dlm)) {
		if (!start_read_lrc) {
			if (line.find(L"[ti") != std::wstring::npos) {
				title_ = TagParser<>::ParseIdTag(L"ti", line);
				XAMP_LOG_DEBUG("title: {}", String::ToUtf8String(title_));
				continue;
			}
			if (line.find(L"[ar") != std::wstring::npos) {
				artist_ = TagParser<>::ParseIdTag(L"ar", line);
				XAMP_LOG_DEBUG("artist: {}", String::ToUtf8String(artist_));
				continue;
			}
			if (line.find(L"[al") != std::wstring::npos) {
				album_ = TagParser<>::ParseIdTag(L"al", line);
				XAMP_LOG_DEBUG("album: {}", String::ToUtf8String(album_));
				continue;
			}
			if (line.find(L"[offset") != std::wstring::npos) {
				offset_ = TagParser<int32_t>::ParseIdTag(L"offset", line);
				XAMP_LOG_DEBUG("offset: {}", offset_);
				continue;
			}
		}		
		if (istr.eof()) {
			ParseLrc(line);
			break;
		}
        auto count = std::count(line.begin(), line.end(), L']');
        if (count == 1) {
            ParseLrc(line);
			start_read_lrc = true;
        } else {
            ParseMultiLrc(line);
			start_read_lrc = true;
        }        
	}

	XAMP_LOG_DEBUG("Total lyrics: {}", lyrics_.size());

	if (lyrics_.empty()) {
		return false;
	}

	std::stable_sort(lyrics_.begin(), lyrics_.end(), [](const LyricEntry &a, const LyricEntry &b) {
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

std::wstring LrcParser::GetMaxLengthLrc() const {
    const auto itr = std::max_element(lyrics_.begin(), lyrics_.end(), [](const LyricEntry& a, const LyricEntry& b) {
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

LyricEntry LrcParser::Last() const {
    return lyrics_.back();
}

LyricEntry LrcParser::LineAt(const int32_t index) const {
    return lyrics_[index];
}

std::chrono::milliseconds LrcParser::GetDuration() const {
    return lyrics_.back().timestamp;
}

int32_t LrcParser::GetInterval() const {
    return GetDuration().count() / lyrics_.size();
}

void LrcParser::AddLrc(const LyricEntry &lrc) {
    lyrics_.push_back(lrc);
}

const LyricEntry& LrcParser::GetLyrics(const std::chrono::milliseconds &time) const noexcept {
    auto itr = BinarySearch(lyrics_.cbegin(), lyrics_.cend(), time, [](const LyricEntry &l, auto time) {
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

int32_t LrcParser::GetSize() const {
    return static_cast<int32_t>(lyrics_.size());
}
