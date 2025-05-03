#include <base/furigana.h>
#include <base/str_utilts.h>
#include <base/unique_handle.h>
#include <sstream>

#include <icu.h>
#include <mecab/mecab.h>

XAMP_BASE_NAMESPACE_BEGIN
namespace {
    struct UTransliteratorDeleter final {
        static UTransliterator* invalid() noexcept {
            return nullptr;
        }

        static void close(UTransliterator* value) {
            if (value != nullptr) {
                ::utrans_close(value);
            }
        }
    };

    using UTransliteratorHandle = UniqueHandle<UTransliterator*, UTransliteratorDeleter>;

    class Kata2HiraConverter {
    public:
        Kata2HiraConverter() {
            UErrorCode status = U_ZERO_ERROR;
            UTransliterator* trans = ::utrans_openU(
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
            ::u_strFromWCS(buffer.data(), buffer.size(), &dest_len, name.data(), name.length(), &status);
            if (U_FAILURE(status)) {
                throw std::runtime_error("Failed to convert name to UChar*");
            }

            const std::wstring unicode_name(reinterpret_cast<wchar_t*>(buffer.data()), dest_len);

            // Perform transliteration
            const int32_t capacity = static_cast<int32_t>(unicode_name.size() * 4 + 10);
            std::vector<UChar> result(capacity);
            int32_t result_length = static_cast<int32_t>(unicode_name.size());
            ::u_memcpy(result.data(), reinterpret_cast<const UChar*>(unicode_name.data()), result_length);

            int32_t limit = result_length;
            ::utrans_transUChars(trans_.get(), result.data(), &result_length, capacity, 0, &limit, &status);
            if (U_FAILURE(status)) {
                throw std::runtime_error("Failed to transliterate name");
            }

            // Convert the result to UTF-8
            int32_t utf8_length = 0;
            ::u_strToUTF8(nullptr, 0, &utf8_length, result.data(), result_length, &status);
            if (status != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(status)) {
                throw std::runtime_error("Failed to get UTF-8 length");
            }

            status = U_ZERO_ERROR;
            std::string utf8_result(utf8_length, '\0');
            ::u_strToUTF8(utf8_result.data(), utf8_length, nullptr, result.data(), result_length, &status);
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

    std::vector<FuriganaEntity> Convert(const std::wstring& text, bool trim_overlapping = false) {
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
                // 在這裡應用 TrimOverlappingSuffix
                if (trim_overlapping) {
					TrimOverlappingSuffix(surface, furigana);
                }
                result.emplace_back(surface, furigana);
            }
            else {
                result.emplace_back(surface);
            }
        }

        // 构建 final_result
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
	std::unique_ptr<MeCab::Tagger> tagger_;
};

Furigana::Furigana()
	: impl_(MakeAlign<FuriganaImpl>()) {
}

XAMP_PIMPL_IMPL(Furigana)

std::vector<FuriganaEntity> Furigana::Convert(const std::wstring& text) {
	return impl_->Convert(text, true);
}

XAMP_BASE_NAMESPACE_END
