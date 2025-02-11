#include <array>
#include <fstream>
#include <regex>

#include <base/text_encoding.h>
#include <widget/util/json_util.h>
#include <widget/util/zib_util.h>
#include <widget/krcparser.h>

namespace {
    const std::array<uint8_t, 16> kKrcFileXorKey = {
		0x40, 0x47, 0x61, 0x77, 0x5E, 0x32, 0x74, 0x47,
		0x51, 0x36, 0x31, 0x2D, 0xCE, 0xD2, 0x6E, 0x69
    };

    constexpr std::array<uint8_t, 4> kKrcFileHead = {
        0x6B, 0x72, 0x63, 0x31
    };

    struct KrcWord {
        // 這個字在此行中的相對起始時間（毫秒）
        std::chrono::milliseconds offset{ 0 };
        // 這個字持續時間（毫秒）
        std::chrono::milliseconds length{ 0 };
        // 通常看到第三個參數都是 0, 也可能是其他值，看需求是否要存
        int extraParam{ 0 };
        // 實際的文字內容，可能是一個字或一小段詞
        std::wstring content;
    };

    struct KrcLine {
        // 這行相對整首歌的起始毫秒
        std::chrono::milliseconds start{ 0 };
        // 這行整體持續毫秒
        std::chrono::milliseconds length{ 0 };
        // 此行內部所有逐字資訊
        std::vector<KrcWord> words;
    };

    std::chrono::milliseconds toMillis(const std::wstring& s) {
        return std::chrono::milliseconds(std::stoi(s));
    }

    // 解析單行 KRC，回傳一個 KrcLine
    KrcLine parseKrcLine(const std::wstring& line) {
        // 先匹配整行的 [行開始毫秒, 行持續毫秒]
        static const std::wregex lineTimeRegex(LR"(^\[(\d+),(\d+)\])");
        // 再匹配行裡所有的 <字offset,字長度,額外>內容
        static const std::wregex wordRegex(LR"(<(\d+),(\d+),(\d+)>([^<]+))");

        KrcLine krcLine;

        // 先找出 "[xxx,yyy]" 這部分
        std::wsmatch matchLine;
        if (std::regex_search(line, matchLine, lineTimeRegex)) {
            // group(1) -> 行開始毫秒
            // group(2) -> 行持續毫秒
            krcLine.start = toMillis(matchLine[1].str());
            krcLine.length = toMillis(matchLine[2].str());
        }
        else {
            // 如果連 [xxx,yyy] 都沒找到，可以視需求決定要丟例外或直接回傳空
            return krcLine;
        }

        // 在整行裡，把 [xxx,yyy] 之後的部分抓出來，繼續解析 <...>標籤
        // matchLine[0] 是整個 [xxx,yyy]；我們要取剩餘部分
        auto remainingStart = matchLine.suffix().first;
        auto remainingEnd = line.end();
        std::wstring remaining(remainingStart, remainingEnd);

        // 用迭代器找出每一個 <offset,length,extra>文字
        std::wsregex_iterator it(remaining.begin(), remaining.end(), wordRegex);
        std::wsregex_iterator end;
        for (; it != end; ++it) {
            KrcWord word;
            word.offset = toMillis((*it)[1].str());     // (1) offset
            word.length = toMillis((*it)[2].str());     // (2) 持續時間
            word.extraParam = std::stoi((*it)[3].str()); // (3) 額外參數
            word.content = (*it)[4].str();              // (4) 文字內容
            krcLine.words.push_back(word);
        }

        return krcLine;
    }

    // 若整個文件含多行，可逐行解析
    std::vector<KrcLine> parseKrc(const std::wstring& wtext) {
        std::vector<KrcLine> krcLines;
        std::wstringstream wss(wtext);
        std::wstring line;

        while (true) {
            if (!std::getline(wss, line)) break;
            if (line.empty()) continue;

            KrcLine kline = parseKrcLine(line);
            // 可以檢查 kline 是否為空行（words.size == 0 && length==0之類）
            // 視需求再決定要不要 push_back
            if (!kline.words.empty()) {
                krcLines.push_back(kline);
            }
        }

        return krcLines;
    }
}

KrcParser::KrcParser() = default;

LyricEntry KrcParser::lineAt(int32_t index) const {
    return lyrics_[index];
}

int32_t KrcParser::getSize() const {
    return static_cast<int32_t>(lyrics_.size());
}

void KrcParser::clear() {
    lyrics_.clear();
}

void KrcParser::addLrc(const LyricEntry& lrc) {
    lyrics_.push_back(lrc);
}

std::vector<LyricEntry>::iterator KrcParser::end() {
    return lyrics_.end();
}

std::vector<LyricEntry>::iterator KrcParser::begin() {
    return lyrics_.begin();
}

std::vector<LyricEntry>::const_iterator KrcParser::cend() const {
    return lyrics_.cend();
}

std::vector<LyricEntry>::const_iterator KrcParser::cbegin() const {
    return lyrics_.cbegin();
}

const LyricEntry& KrcParser::getLyrics(const std::chrono::milliseconds& time) const noexcept {
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

bool KrcParser::parse(std::wistream& istr) {
    clear();
    std::wstringstream buffer;
    buffer << istr.rdbuf();
    std::wstring content = buffer.str();
	return parseKrcText(content);
}

bool KrcParser::parse(const uint8_t* buffer, size_t size) {
    if (size <= kKrcFileHead.size()) {
        return false;
    }

    for (int i = 0; i < kKrcFileHead.size(); i++) {
        if (buffer[i] != kKrcFileHead[i]) {
            return false;
        }
    }

    const size_t content_size = size - kKrcFileHead.size();
    if (content_size == 0) {
        return false;
    }

    std::vector<uint8_t> encoded_data(content_size);
    std::memcpy(encoded_data.data(), &buffer[4], content_size);

    for (size_t i = 0; i < encoded_data.size(); i++) {
        encoded_data[i] = encoded_data[i] ^ kKrcFileXorKey[i % 16];
    }

    std::string decompressed;
    if (!decompress(encoded_data.data(), 
        encoded_data.size(), decompressed)) {
        return false;
    }

    TextEncoding encoding;
    auto utf8_str = encoding.ToUtf8String(decompressed,
        decompressed.length(), false);
    const auto wtext = String::ToStdWString(utf8_str);
    return parseKrcText(wtext);
}

bool KrcParser::parseFile(const std::wstring& file_path) {
    std::ifstream ifs(file_path, std::ios::binary);
    if (!ifs.is_open()) {
		return false;
    }

	std::vector<uint8_t> file_data(
        (std::istreambuf_iterator<char>(ifs)),
        std::istreambuf_iterator<char>());
    ifs.close();

	return parse(file_data.data(), file_data.size());
}

bool KrcParser::parseKrcText(const std::wstring& wtext) {
    auto lines = parseKrc(wtext);

    size_t totalWords = 0;
    for (auto& line : lines) {
        totalWords += line.words.size();
    }

    lyrics_.reserve(totalWords);
    int32_t idx = 0;

    for (const auto& line : lines) {
        LyricEntry entry;
        entry.index = idx++;
        entry.timestamp = line.start;
        entry.start_time = entry.timestamp;
		for (const auto& word : line.words) {
			entry.lrc += word.content;
		}
        std::chrono::milliseconds max_end(0);
        for (const auto& word : line.words) {
			entry.words.push_back(
                LyricWord {
                	word.offset, word.length, 0, word.content });
            auto word_end = word.offset + word.length;
            if (word_end > max_end) {
                max_end = word_end;
            }
        }
        entry.end_time = entry.start_time + max_end;
        lyrics_.push_back(entry);
    }

    std::sort(lyrics_.begin(), lyrics_.end(), [](auto& a, auto& b) {
        return a.timestamp < b.timestamp;
        });

    return !lyrics_.empty();
}

QList<InfoItem> parseInfoData(const QString& jsonString) {
    QList<InfoItem> results;
    QJsonParseError json_error;
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8(), &json_error);
    if (json_error.error != QJsonParseError::NoError) {
		return results;
    }
    QJsonObject root = doc.object();
    if (!root.contains("data"_str)) {
		return results;
    }
    QJsonObject data_obj = root.value("data"_str).toObject();
    if (!data_obj.contains("info"_str)) {
		return results;
    }
    QJsonValue info_value = data_obj.value("info"_str);
    if (!info_value.isArray()) {
        return results;
    }
    QJsonArray info_array = info_value.toArray();
	for (const auto& item : info_array) {
		QJsonObject obj = item.toObject();
		InfoItem info;
		info.hash = obj.value("hash"_str).toString();
		info.sqhash = obj.value("sqhash"_str).toString();
		info.songname = obj.value("songname"_str).toString();
		info.albumName = obj.value("albumName"_str).toString();
		info.singername = obj.value("singername"_str).toString();
		info.duration = obj.value("duration"_str).toInt();
		results.push_back(info);
	}
    return results;
}

QList<Candidate> parseCandidatesFromJson(const QString& jsonString) {
    QList<Candidate> result;
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
		return result;
    }
    if (!doc.isObject()) {
        return result;
    }
    QJsonObject root_obj = doc.object();
    if (!root_obj.contains("candidates"_str) 
        || !root_obj.value("candidates"_str).isArray()) {
        return result;
    }
    QJsonArray c_array = root_obj.value("candidates"_str).toArray();
    for (int i = 0; i < c_array.size(); ++i) {
        QJsonValue val = c_array.at(i);
        if (!val.isObject()) {
            continue;
        }
        QJsonObject cObj = val.toObject();
        Candidate c;
        c.id = cObj.value("id"_str).toString();
        c.accesskey = cObj.value("accesskey"_str).toString();
        c.singer = cObj.value("singer"_str).toString();
        c.song = cObj.value("song"_str).toString();
        result.append(c);
    }
    return result;
}

std::optional<KrcContent> parseKrcContent(const QString& jsonString) {
    KrcContent result;
    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8(), &parse_error);
    if (parse_error.error != QJsonParseError::NoError) {
        return std::nullopt;
    }
    if (!doc.isObject()) {
        return std::nullopt;
    }
    QJsonObject root_obj = doc.object();
    result.base64Content = root_obj.value("content"_str).toString();
    result.decodedContent = QByteArray::fromBase64(result.base64Content.toUtf8());
	return MakeOptional<KrcContent>(result);
}
