#include <base/furigana.h>
#include <base/str_utilts.h>
#include <base/unique_handle.h>
#include <base/encoding_detector_tables.h>
#include <sstream>

#include <icu.h>
#include <mecab/mecab.h>

XAMP_BASE_NAMESPACE_BEGIN
namespace {
    struct UCharsetDetectorDeleter final {
        static UCharsetDetector* invalid() noexcept {
            return nullptr;
        }

        static void close(UCharsetDetector* value) {
            if (value != nullptr) {
                ucsdet_close(value);
            }
        }
    };

    using UCharsetDetectorHandle = UniqueHandle<UCharsetDetector*, UCharsetDetectorDeleter>;

    struct UTransliteratorDeleter final {
        static UTransliterator* invalid() noexcept {
            return nullptr;
        }

        static void close(UTransliterator* value) {
            if (value != nullptr) {
                utrans_close(value);
            }
        }
    };

    using UTransliteratorHandle = UniqueHandle<UTransliterator*, UTransliteratorDeleter>;

    class CharsetDetector {
    public:
		CharsetDetector() {
			UErrorCode status = U_ZERO_ERROR;
			UCharsetDetector* detector = ucsdet_open(&status);
			if (U_FAILURE(status)) {
				throw std::runtime_error("Failed to create UCharsetDetector");
			}
			detector_handle_.reset(detector);
		}

        bool IsFrequent(const uint16_t* values, uint32_t c) {
            int start = 0;
            int end = 511; // All the tables have 512 entries
            int mid = (start + end) / 2;
            while (start <= end) {
                if (c == values[mid]) {
                    return true;
                }
                else if (c > values[mid]) {
                    start = mid + 1;
                }
                else {
                    end = mid - 1;
                }
                mid = (start + end) / 2;
            }
            return false;
        }

        const UCharsetMatch* GetPreferred(
            const char* input, size_t len,
            const UCharsetMatch** ucma, size_t nummatches,
            bool* goodmatch, int* highestmatch) {
            *goodmatch = false;
            std::vector<const UCharsetMatch*> matches;
            UErrorCode status = U_ZERO_ERROR;
            for (size_t i = 0; i < nummatches; i++) {
                const char* encname = ucsdet_getName(ucma[i], &status);
                int confidence = ucsdet_getConfidence(ucma[i], &status);
                matches.push_back(ucma[i]);
            }
            size_t num = matches.size();
            if (num == 0) {
                return nullptr;
            }
            if (num == 1) {
                int confidence = ucsdet_getConfidence(matches[0], &status);
                if (confidence > 15) {
                    *goodmatch = true;
                }
                return matches[0];
            }
            // keep track of how many "special" characters result when converting the input using each
            // encoding
            std::vector<int> newconfidence;
            for (size_t i = 0; i < num; i++) {
                const uint16_t* freqdata = nullptr;
                float freqcoverage = 0;
                status = U_ZERO_ERROR;
                const char* encname = ucsdet_getName(matches[i], &status);
                int confidence = ucsdet_getConfidence(matches[i], &status);
                if (!strcmp("GB18030", encname)) {
                    freqdata = frequent_zhCN;
                    freqcoverage = frequent_zhCN_coverage;
                }
                else if (!strcmp("Big5", encname)) {
                    freqdata = frequent_zhTW;
                    freqcoverage = frequent_zhTW_coverage;
                }
                else if (!strcmp("EUC-KR", encname)) {
                    freqdata = frequent_ko;
                    freqcoverage = frequent_ko_coverage;
                }
                else if (!strcmp("EUC-JP", encname)) {
                    freqdata = frequent_ja;
                    freqcoverage = frequent_ja_coverage;
                }
                else if (!strcmp("Shift_JIS", encname)) {
                    freqdata = frequent_ja;
                    freqcoverage = frequent_ja_coverage;
                }
                status = U_ZERO_ERROR;
                UConverter* conv = ucnv_open(encname, &status);
                int demerit = 0;
                if (U_FAILURE(status)) {
                    confidence = 0;
                    demerit += 1000;
                }
                const char* source = input;
                const char* sourceLimit = input + len;
                status = U_ZERO_ERROR;
                int frequentchars = 0;
                int totalchars = 0;
                while (true) {
                    // demerit the current encoding for each "special" character found after conversion.
                    // The amount of demerit is somewhat arbitrarily chosen.
                    UChar32 c = ucnv_getNextUChar(conv, &source, sourceLimit, &status);
                    if (!U_SUCCESS(status)) {
                        break;
                    }
                    if (c < 0x20 || (c >= 0x7f && c <= 0x009f)) {
                        demerit += 100;
                    }
                    else if ((c == 0xa0)                      // no-break space
                        || (c >= 0xa2 && c <= 0xbe)         // symbols, superscripts
                        || (c == 0xd7) || (c == 0xf7)       // multiplication and division signs
                        || (c >= 0x2000 && c <= 0x209f)) {  // punctuation, superscripts
                        demerit += 10;
                    }
                    else if (c >= 0xe000 && c <= 0xf8ff) {
                        demerit += 30;
                    }
                    else if (c >= 0x2190 && c <= 0x2bff) {
                        // this range comprises various symbol ranges that are unlikely to appear in
                        // music file metadata.
                        demerit += 10;
                    }
                    else if (c == 0xfffd) {
                        demerit += 50;
                    }
                    else if (c >= 0xfff0 && c <= 0xfffc) {
                        demerit += 50;
                    }
                    else if (freqdata != nullptr) {
                        totalchars++;
                        if (IsFrequent(freqdata, c)) {
                            frequentchars++;
                        }
                    }
                }
                if (freqdata != nullptr && totalchars != 0) {
                    int myconfidence = 10 + float((100 * frequentchars) / totalchars) / freqcoverage;
                    if (myconfidence > 100) myconfidence = 100;
                    if (myconfidence < 0) myconfidence = 0;
                    confidence = myconfidence;
                }

                newconfidence.push_back(confidence - demerit);
                ucnv_close(conv);
                if (i == 0 && (confidence - demerit) == 100) {
                    // no need to check any further, we'll end up using this match anyway
                    break;
                }
            }
            // find match with highest confidence after adjusting for unlikely characters
            int highest = newconfidence[0];
            size_t highestidx = 0;
            int runnerup = -10000;
            int runnerupidx = -10000;
            num = newconfidence.size();
            for (size_t i = 1; i < num; i++) {
                if (newconfidence[i] > highest) {
                    runnerup = highest;
                    runnerupidx = highestidx;
                    highest = newconfidence[i];
                    highestidx = i;
                }
                else if (newconfidence[i] > runnerup) {
                    runnerup = newconfidence[i];
                    runnerupidx = i;
                }
            }

            status = U_ZERO_ERROR;

            if (runnerupidx < 0) {
                if (highest > 15) {
                    *goodmatch = true;
                }
            }
            else {
                if (runnerup < 0) {
                    runnerup = 0;
                }
                if ((highest - runnerup) > 15) {
                    *goodmatch = true;
                }
            }
            *highestmatch = highest;
            return matches[highestidx];
        }

		void Detect(const std::string_view& text) {
			UErrorCode status = U_ZERO_ERROR;
			ucsdet_setText(detector_handle_.get(), text.data(), text.size(), &status);
			if (U_FAILURE(status)) {
				throw std::runtime_error("Failed to set text to UCharsetDetector");
			}
            int32_t matches;
			const auto** match = ucsdet_detectAll(detector_handle_.get(), &matches, &status);
			if (U_FAILURE(status)) {
				throw std::runtime_error("Failed to detect charset");
			}

            bool goodmatch = true;
            int highest = 0;
            auto *hiest_match = GetPreferred(text.data(), text.length(), match, matches, &goodmatch, &highest);

            charset_ = ucsdet_getName(hiest_match, &status);
            if (U_FAILURE(status)) {
                throw std::runtime_error("Failed to get charset name");
            }

            lang_ = ucsdet_getLanguage(hiest_match, &status);
            if (U_FAILURE(status)) {
                throw std::runtime_error("Failed to get language");
            }
		}

		std::string GetCharset() const {
			return charset_;
		}

		std::string GetLanguage() const {
			return lang_;
		}

    private:
        std::string charset_;
		std::string lang_;
        UCharsetDetectorHandle detector_handle_;
    };

    class Kata2HiraConverter {
    public:
        Kata2HiraConverter() {
            UErrorCode status = U_ZERO_ERROR;
            UTransliterator* trans = utrans_openU(
                u"Katakana-Hiragana",
                -1,
                UTRANS_FORWARD,
                nullptr,
                -1,
                nullptr,
                &status
            );
            if (U_FAILURE(status)) {
                throw std::runtime_error("Failed to create UTransliterator");
            }
            trans_.reset(trans);
        }

        std::wstring Convert(const std::wstring_view& name) {
            UErrorCode status = U_ZERO_ERROR;

            // Convert std::wstring to UChar*
            std::vector<UChar> buffer(name.length());
            int32_t dest_len = 0;
            u_strFromWCS(buffer.data(), buffer.size(), &dest_len, name.data(), name.length(), &status);
            if (U_FAILURE(status)) {
                throw std::runtime_error("Failed to convert name to UChar*");
            }

            const std::wstring unicode_name(reinterpret_cast<wchar_t*>(buffer.data()), dest_len);

            // Perform transliteration
            const int32_t capacity = static_cast<int32_t>(unicode_name.size() * 4 + 10);
            std::vector<UChar> result(capacity);
            int32_t result_length = static_cast<int32_t>(unicode_name.size());
            u_memcpy(result.data(), reinterpret_cast<const UChar*>(unicode_name.data()), result_length);

            int32_t limit = result_length;
            utrans_transUChars(trans_.get(), result.data(), &result_length, capacity, 0, &limit, &status);
            if (U_FAILURE(status)) {
                throw std::runtime_error("Failed to transliterate name");
            }

            // Convert the result to UTF-8
            int32_t utf8_length = 0;
            u_strToUTF8(nullptr, 0, &utf8_length, result.data(), result_length, &status);
            if (status != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(status)) {
                throw std::runtime_error("Failed to get UTF-8 length");
            }

            status = U_ZERO_ERROR;
            std::string utf8_result(utf8_length, '\0');
            u_strToUTF8(utf8_result.data(), utf8_length, nullptr, result.data(), result_length, &status);
            if (U_FAILURE(status)) {
                throw std::runtime_error("Failed to convert result to UTF-8");
            }

			return String::ToStdWString(utf8_result);
        }

    private:
        UTransliteratorHandle trans_;
    };

    // Function to trim overlapping suffix between text and furigana
    void TrimOverlappingSuffix(std::wstring& text, std::wstring& furigana) {
        size_t text_len = text.length();
        size_t furigana_len = furigana.length();
        size_t min_len = (std::min)(text_len, furigana_len);

        size_t overlap_length = 0;

        // Compare characters from the end to find the overlapping suffix
        for (size_t i = 1; i <= min_len; ++i) {
            if (text[text_len - i] == furigana[furigana_len - i]) {
                overlap_length = i;
            }
            else {
                break;
            }
        }

        // If there is an overlapping suffix, remove it from both text and furigana
        if (overlap_length > 0) {
            text.erase(text_len - overlap_length, overlap_length);
            furigana.erase(furigana_len - overlap_length, overlap_length);
        }
    }

    bool IsKanji(wchar_t c) {
        return (c >= 0x4E00 && c <= 0x9FAF);
    }

    bool HasKanji(const std::wstring& kata) {
        return std::any_of(kata.begin(), kata.end(), [](wchar_t c) {
            return IsKanji(c);
            });
    }

    bool IsAscii(const std::wstring& text) {
        return std::all_of(text.begin(), text.end(), [](auto c) {
            return isascii(c);
            });
    }
}

class Furigana::FuriganaImpl {
public:
	FuriganaImpl() {
		tagger_.reset(MeCab::createTagger("-Ochasen"));
        tagger_->parse(""); // Initializes MeCab parser
	}

    std::vector<FuriganaEntity> ConvertV3(const std::wstring& text, bool trim_overlapping = false) {
        if (text.empty()) {
            return {};
        }
        if (IsAscii(text)) {
            return { FuriganaEntity{ text } };
        }

        std::vector<FuriganaEntity> result;
        result.reserve(text.size());

        const auto utf8 = String::ToUtf8String(text);
        const auto* node = tagger_->parseToNode(utf8.c_str());

        for (; node != nullptr; node = node->next) {
            if (node->stat == MECAB_BOS_NODE || node->stat == MECAB_EOS_NODE) {
                continue;
            }

            auto surface = String::ToStdWString(std::string(node->surface, node->length));
            auto feature = String::ToStdWString(node->feature);

            auto features = String::Split<wchar_t>(feature, L",");

            if (!HasKanji(surface) || features.size() <= 7 || features[7].empty()) {
                result.emplace_back(surface);
                continue;
            }

            auto furigana = converter_.Convert(features[7]);
            if (surface != furigana) {
                // �b�o������ TrimOverlappingSuffix
                if (trim_overlapping) {
					TrimOverlappingSuffix(surface, furigana);
                }
                result.emplace_back(surface, furigana);
            }
            else {
                result.emplace_back(surface);
            }
        }

        // �۫� final_result
        std::vector<FuriganaEntity> final_result;
        size_t current_pos = 0;
        for (const auto& entity : result) {
            size_t pos = text.find(entity.text, current_pos);
            if (pos != std::wstring::npos) {
                if (pos > current_pos) {
                    auto unanalyzed_text = text.substr(current_pos, pos - current_pos);
                    final_result.emplace_back(unanalyzed_text);
                }
                final_result.push_back(entity);
                current_pos = pos + entity.text.length();
            }
            else {
                final_result.push_back(entity);
            }
        }

        if (current_pos < text.length()) {
            auto unanalyzed_text = text.substr(current_pos);
            final_result.emplace_back(unanalyzed_text);
        }

        return final_result;
    }

    Kata2HiraConverter converter_;
	CharsetDetector detector_;
	std::unique_ptr<MeCab::Tagger> tagger_;
};

Furigana::Furigana()
	: impl_(MakeAlign<FuriganaImpl>()) {
}

XAMP_PIMPL_IMPL(Furigana)

std::vector<FuriganaEntity> Furigana::Convert(const std::wstring& text) {
	return impl_->ConvertV3(text, true);
}

XAMP_BASE_NAMESPACE_END
