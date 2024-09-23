#include <base/nameconverter.h>
#include <base/unique_handle.h>
#include <base/str_utilts.h>
#include <base/charset_detector.h>

#include <icucommon.h>

#include <icu.h>
#include <stdexcept>
#include <vector>


XAMP_BASE_NAMESPACE_BEGIN

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

class NameConverter::NameConverterImpl {
public:
    NameConverterImpl() {
        UErrorCode status = U_ZERO_ERROR;
        const UChar chinese_id[] = L"Han-Latin/Names; [:Nonspacing Mark:] Remove; Any-Upper";
        chinese_trans_.reset(utrans_openU(chinese_id, -1, UTRANS_FORWARD, nullptr, -1, nullptr, &status));

    	status = U_ZERO_ERROR;
        const UChar japanese_id[] = L"Katakana-Latin; Hiragana-Latin; Han-Latin/Names; [:Nonspacing Mark:] Remove; Any-Upper";
        japanese_trans_.reset(utrans_openU(japanese_id, -1, UTRANS_FORWARD, nullptr, -1, nullptr, &status));
    }

    std::string ConvertName(const std::wstring& name, LanguageType lang) {
        UErrorCode status = U_ZERO_ERROR;

        // Convert std::wstring to UChar*
        std::vector<UChar> buffer(name.length());
        int32_t dest_len = 0;
        u_strFromWCS(buffer.data(), buffer.size(), &dest_len, name.c_str(), name.length(), &status);
        if (U_FAILURE(status)) {
            throw std::runtime_error("Failed to convert name to UChar*");
        }

        std::wstring unicode_name(buffer.data(), dest_len);

        // Choose the appropriate transliterator
        UTransliterator* trans = nullptr;
        if (lang == LanguageType::LANGUAGE_CHINESE) {
            trans = chinese_trans_.get();
        }
        else if (lang == LanguageType::LANGUAGE_JAPANESE) {
            trans = japanese_trans_.get();
        }
        else if (lang == LanguageType::LANGUAGE_ENGLISH) {
            // For English names, convert directly to uppercase
            std::string upper_name;
            for (wchar_t ch : name) {
                upper_name += std::toupper(static_cast<unsigned char>(ch));
            }
            return upper_name;
        }
        else {
            // Unknown language, return the original name
            std::string original_name(name.begin(), name.end());
            return original_name;
        }

        if (!trans) {
            throw std::runtime_error("Transliterator not initialized");
        }

        // Perform transliteration
        int32_t capacity = static_cast<int32_t>(unicode_name.size() * 4 + 10);
        std::vector<UChar> result(capacity);
        int32_t result_length = static_cast<int32_t>(unicode_name.size());
        u_memcpy(result.data(), unicode_name.data(), result_length);

        int32_t limit = result_length;
        utrans_transUChars(trans, result.data(), &result_length, capacity, 0, &limit, &status);
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

        return utf8_result;
    }

    char GetInitialLetter(const std::wstring& name, LanguageType lang) {
	    const std::string converted_name = ConvertName(name, lang);

        for (char ch : converted_name) {
            if (std::isalpha(static_cast<unsigned char>(ch))) {
                return static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            }
        }
        return '\0';
    }
private:
    UTransliteratorHandle chinese_trans_;
    UTransliteratorHandle japanese_trans_;
};

NameConverter::NameConverter()
	: impl_(new NameConverterImpl()) {
}

XAMP_PIMPL_IMPL(NameConverter)

std::string NameConverter::ConvertName(const std::wstring& name, LanguageType lang) {
	return impl_->ConvertName(name, lang);
}

char NameConverter::GetInitialLetter(const std::wstring& name, LanguageType lang) {
	return impl_->GetInitialLetter(name, lang);
}

XAMP_BASE_NAMESPACE_END