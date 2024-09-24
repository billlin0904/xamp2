#include <base/transliterator.h>
#include <base/unique_handle.h>
#include <base/str_utilts.h>
#include <base/charset_detector.h>

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

class Transliterator::TransliteratorImpl {
public:
    TransliteratorImpl() {
        UErrorCode status = U_ZERO_ERROR;
        const UChar id[] = u"Katakana-Latin; Hiragana-Latin; Han-Latin/Names; [:Nonspacing Mark:] Remove; Any-Upper";
        //const UChar id[] = u"Katakana-Latin; Hiragana-Latin; [:Nonspacing Mark:] Remove; Any-Upper";
        trans_.reset(utrans_openU(id, -1, UTRANS_FORWARD, nullptr, -1, nullptr, &status));
    }

    std::string TransformToLatin(const std::wstring& name) {
        UErrorCode status = U_ZERO_ERROR;

        // Convert std::wstring to UChar*
        std::vector<UChar> buffer(name.length());
        int32_t dest_len = 0;
        u_strFromWCS(buffer.data(), buffer.size(), &dest_len, name.c_str(), name.length(), &status);
        if (U_FAILURE(status)) {
            throw std::runtime_error("Failed to convert name to UChar*");
        }

        std::wstring unicode_name(reinterpret_cast<wchar_t*>(buffer.data()), dest_len);

        // Perform transliteration
        int32_t capacity = static_cast<int32_t>(unicode_name.size() * 4 + 10);
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

        return utf8_result;
    }

    char GetLatinLetter(const std::wstring& name) {
	    const std::string converted_name = TransformToLatin(name);

        for (char ch : converted_name) {
            if (std::isalpha(static_cast<unsigned char>(ch))) {
                return static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            }
        }
        return '\0';
    }
private:
    UTransliteratorHandle trans_;
};

Transliterator::Transliterator()
	: impl_(new TransliteratorImpl()) {
}

XAMP_PIMPL_IMPL(Transliterator)

std::string Transliterator::TransformToLatin(const std::wstring& name) {
	return impl_->TransformToLatin(name);
}

char Transliterator::GetLatinLetter(const std::wstring& name) {
	return impl_->GetLatinLetter(name);
}

XAMP_BASE_NAMESPACE_END