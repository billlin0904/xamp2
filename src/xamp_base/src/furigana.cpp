#include <base/furigana.h>
#include <base/dll.h>
#include <base/fs.h>
#include <base/shared_singleton.h>
#include <base/str_utilts.h>
#include <base/unique_handle.h>
#include <sstream>
#include <limits>

#include <unicode/utrans.h>
#include <unicode/ustring.h>
#include <unicode/uvernum.h>
#ifdef XAMP_OS_LINUX
#include <mecab.h>
#else
#include <mecab/mecab.h>
#endif

XAMP_BASE_NAMESPACE_BEGIN
namespace {
    constexpr std::string_view getICUCommonLibraryName() noexcept {
#if defined(XAMP_OS_WIN) && defined(_DEBUG)
        return "icuucd" U_ICU_VERSION_SHORT;
#elif defined(XAMP_OS_WIN)
        return "icuuc" U_ICU_VERSION_SHORT;
#elif defined(XAMP_OS_LINUX)
        return "icuuc-" U_ICU_VERSION_SHORT;
#else
        return "icuuc";
#endif
    }

    constexpr std::string_view getICUTransLibraryName() noexcept {
#if defined(XAMP_OS_WIN) && defined(_DEBUG)
        return "icuind" U_ICU_VERSION_SHORT;
#elif defined(XAMP_OS_WIN)
        return "icuin" U_ICU_VERSION_SHORT;
#elif defined(XAMP_OS_LINUX)
        return "icuin-" U_ICU_VERSION_SHORT;
#else
        return "icuin";
#endif
    }

#define XAMP_ICU_STRINGIZE_IMPL(Func) #Func
#define XAMP_ICU_STRINGIZE(Func) XAMP_ICU_STRINGIZE_IMPL(Func)

    class ICUCommonLib final {
    public:
        XAMP_DECLARE_SINGLETON_NAME()

        ICUCommonLib()
            : module_(openSharedLibrary(getICUCommonLibraryName()))
            , u_strFromWCS(module_, XAMP_ICU_STRINGIZE(u_strFromWCS))
            , u_memcpy(module_, XAMP_ICU_STRINGIZE(u_memcpy))
            , u_strToUTF8(module_, XAMP_ICU_STRINGIZE(u_strToUTF8)) {
        }

        XAMP_DISABLE_COPY(ICUCommonLib)

    private:
        SharedLibraryHandle module_;

    public:
        XAMP_DECLARE_DLL_NAME(u_strFromWCS);
        XAMP_DECLARE_DLL_NAME(u_memcpy);
        XAMP_DECLARE_DLL_NAME(u_strToUTF8);
    };

    class ICUTransLib final {
    public:
        XAMP_DECLARE_SINGLETON_NAME()

        ICUTransLib()
            : module_(openSharedLibrary(getICUTransLibraryName()))
            , utrans_openU(module_, XAMP_ICU_STRINGIZE(utrans_openU))
            , utrans_close(module_, XAMP_ICU_STRINGIZE(utrans_close))
            , utrans_transUChars(module_, XAMP_ICU_STRINGIZE(utrans_transUChars)) {
        }

        XAMP_DISABLE_COPY(ICUTransLib)

    private:
        SharedLibraryHandle module_;

    public:
        XAMP_DECLARE_DLL_NAME(utrans_openU);
        XAMP_DECLARE_DLL_NAME(utrans_close);
        XAMP_DECLARE_DLL_NAME(utrans_transUChars);
    };

#undef XAMP_ICU_STRINGIZE
#undef XAMP_ICU_STRINGIZE_IMPL

#define ICU_COMMON_LIB SharedSingleton<ICUCommonLib>::getInstance()
#define ICU_TRANS_LIB SharedSingleton<ICUTransLib>::getInstance()

    class MeCabLib final {
    public:
        XAMP_DECLARE_SINGLETON_NAME()

        MeCabLib()
            : module_(openSharedLibrary("mecab"))
            , XAMP_LOAD_DLL_API(mecab_new)
            , XAMP_LOAD_DLL_API(mecab_new2)
            , XAMP_LOAD_DLL_API(mecab_strerror)
            , XAMP_LOAD_DLL_API(mecab_destroy)
            , XAMP_LOAD_DLL_API(mecab_sparse_tostr)
            , XAMP_LOAD_DLL_API(mecab_sparse_tonode) {
        }

        XAMP_DISABLE_COPY(MeCabLib)

    private:
        SharedLibraryHandle module_;

    public:
        XAMP_DECLARE_DLL_NAME(mecab_new);
        XAMP_DECLARE_DLL_NAME(mecab_new2);
        XAMP_DECLARE_DLL_NAME(mecab_strerror);
        XAMP_DECLARE_DLL_NAME(mecab_destroy);
        XAMP_DECLARE_DLL_NAME(mecab_sparse_tostr);
        XAMP_DECLARE_DLL_NAME(mecab_sparse_tonode);
    };

#define MECAB_LIB SharedSingleton<MeCabLib>::getInstance()

    struct MeCabTaggerDeleter final {
        static mecab_t* invalid() {
            return nullptr;
        }

        static void close(mecab_t* value) {
            if (value != nullptr) {
                MECAB_LIB.mecab_destroy(value);
            }
        }
    };

    using MeCabTaggerHandle = UniqueHandle<mecab_t*, MeCabTaggerDeleter>;

    std::string getMeCabError(mecab_t* tagger) {
        const auto* error = MECAB_LIB.mecab_strerror(tagger);
        if (error == nullptr) {
            return "Unknown MeCab error";
        }
        return error;
    }

    std::vector<std::string> getMeCabArguments() {
        std::vector<std::string> args{
            "xamp",
            "-Ochasen"
        };

        const auto mecab_dir = getApplicationFilePath() / "mecab";
        const auto mecabrc_path = mecab_dir / "mecabrc";
        if (Fs::exists(mecabrc_path)) {
            args.emplace_back("-r");
            args.emplace_back(mecabrc_path.string());
        }

        const auto dic_dir = mecab_dir / "dic";
        if (Fs::exists(dic_dir)) {
            args.emplace_back("-d");
            args.emplace_back(dic_dir.string());
        }
        return args;
    }

    std::string joinMeCabArguments(const std::vector<std::string>& args) {
        std::string result;
        for (const auto& arg : args) {
            if (!result.empty()) {
                result += ' ';
            }
            result += arg;
        }
        return result;
    }

    struct UTransliteratorDeleter final {
        static UTransliterator* invalid() {
            return nullptr;
        }

        static void close(UTransliterator* value) {
            if (value != nullptr) {
                ICU_TRANS_LIB.utrans_close(value);
            }
        }
    };

    using UTransliteratorHandle = UniqueHandle<UTransliterator*, UTransliteratorDeleter>;

    class Kata2HiraConverter {
    public:
        Kata2HiraConverter() {
            UErrorCode status = U_ZERO_ERROR;
            UTransliterator* trans = ICU_TRANS_LIB.utrans_openU(
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

        std::wstring convert(const std::wstring_view& name) {
            UErrorCode status = U_ZERO_ERROR;

            if (name.length() > static_cast<size_t>((std::numeric_limits<int32_t>::max)())) {
                throw std::runtime_error("Name is too long to transliterate");
            }

            const auto source_length = static_cast<int32_t>(name.length());
            std::vector<UChar> buffer(source_length + 1);
            int32_t dest_len = 0;
            ICU_COMMON_LIB.u_strFromWCS(buffer.data(), static_cast<int32_t>(buffer.size()), &dest_len, name.data(), source_length, &status);
            if (U_FAILURE(status)) {
                throw std::runtime_error("Failed to convert name to UChar*");
            }

            // Perform transliteration
            if (dest_len > ((std::numeric_limits<int32_t>::max)() - 10) / 4) {
                throw std::runtime_error("Name is too long to transliterate");
            }
            const int32_t capacity = dest_len * 4 + 10;
            std::vector<UChar> result(capacity);
            int32_t result_length = dest_len;
            ICU_COMMON_LIB.u_memcpy(result.data(), buffer.data(), result_length);

            int32_t limit = result_length;
            ICU_TRANS_LIB.utrans_transUChars(trans_.get(), result.data(), &result_length, capacity, 0, &limit, &status);
            if (U_FAILURE(status)) {
                throw std::runtime_error("Failed to transliterate name");
            }

            // convert the result to UTF-8
            int32_t utf8_length = 0;
            ICU_COMMON_LIB.u_strToUTF8(nullptr, 0, &utf8_length, result.data(), result_length, &status);
            if (status != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(status)) {
                throw std::runtime_error("Failed to get UTF-8 length");
            }

            status = U_ZERO_ERROR;
            std::string utf8_result(utf8_length, '\0');
            ICU_COMMON_LIB.u_strToUTF8(utf8_result.data(), utf8_length, nullptr, result.data(), result_length, &status);
            if (U_FAILURE(status)) {
                throw std::runtime_error("Failed to convert result to UTF-8");
            }

			return String::toStdWString(utf8_result);
        }

    private:
        UTransliteratorHandle trans_;
    };

    // Function to trim overlapping suffix between text and furigana
    void trimOverlappingSuffix(std::wstring& text, std::wstring& furigana) {
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

    bool isKanji(wchar_t c) {
        return (c >= 0x4E00 && c <= 0x9FAF);
    }

    bool hasKanji(const std::wstring& kata) {
        return std::any_of(kata.begin(), kata.end(), [](wchar_t c) {
            return isKanji(c);
            });
    }

    bool isAscii(const std::wstring& text) {
        return std::all_of(text.begin(), text.end(), [](auto c) {
            return isascii(c);
            });
    }
}

class Furigana::FuriganaImpl {
public:
	FuriganaImpl() {
        auto args = getMeCabArguments();
        std::vector<char*> argv;
        argv.reserve(args.size());
        for (auto& arg : args) {
            argv.push_back(arg.data());
        }

		tagger_.reset(MECAB_LIB.mecab_new(static_cast<int>(argv.size()), argv.data()));
        if (!tagger_) {
            auto error = getMeCabError(nullptr);
            if (error.empty()) {
                error = "MeCab initialization failed: " + joinMeCabArguments(args);
            }
            throw std::runtime_error(error);
        }
        if (MECAB_LIB.mecab_sparse_tostr(tagger_.get(), "") == nullptr) {
            throw std::runtime_error(getMeCabError(tagger_.get()));
        }
	}

    std::vector<FuriganaEntity> convert(const std::wstring& text, bool trim_overlapping = false) {
        if (text.empty()) {
            return {};
        }
        if (isAscii(text)) {
            return { FuriganaEntity{ text } };
        }

        std::vector<FuriganaEntity> result;
        result.reserve(text.size());

        const auto utf8 = String::toUtf8String(text);
        const auto* node = MECAB_LIB.mecab_sparse_tonode(tagger_.get(), utf8.c_str());

        for (; node != nullptr; node = node->next) {
            if (node->stat == MECAB_BOS_NODE || node->stat == MECAB_EOS_NODE) {
                continue;
            }

            auto surface = String::toStdWString(std::string(node->surface, node->length));
            auto feature = String::toStdWString(node->feature);

            auto features = String::split<wchar_t>(feature, L",");

            if (!hasKanji(surface) || features.size() <= 7 || features[7].empty()) {
                result.emplace_back(surface);
                continue;
            }

            auto furigana = converter_.convert(features[7]);
            if (surface != furigana) {
                if (trim_overlapping) {
					trimOverlappingSuffix(surface, furigana);
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
	MeCabTaggerHandle tagger_;
};

Furigana::Furigana()
	: impl_(makeAlign<FuriganaImpl>()) {
}

XAMP_PIMPL_IMPL(Furigana)

std::vector<FuriganaEntity> Furigana::convert(const std::wstring& text) {
	return impl_->convert(text, true);
}

void loadFuriganaDll() {
    ICU_COMMON_LIB;
    ICU_TRANS_LIB;
    MECAB_LIB;
}

XAMP_BASE_NAMESPACE_END
