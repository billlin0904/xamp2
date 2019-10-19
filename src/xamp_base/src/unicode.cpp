#include <locale.h>
#include <locale>
#include <stdlib.h>

#ifdef __APPLE__
#include <xlocale.h>
#else
#include <Windows.h>
#endif

#include <base/unicode.h>

namespace xamp::base {

#ifndef _WIN32
struct LocalHelper {
    LocalHelper()
        : l(newlocale(LC_CTYPE_MASK, "en_US.UTF-8", LC_GLOBAL_LOCALE)) {
    }

    ~LocalHelper() {
        freelocale(l);
    }

    std::wstring ToStdWString(const std::string &utf8) {
        auto size = mbstowcs_l(nullptr, utf8.c_str(), 0, l);
        if (!size) {
            return L"";
        }
        std::wstring ret(size, 0);
        mbstowcs_l(ret.data(), utf8.c_str(), size, l);
        return ret;
    }

    std::string ToUtf8String(const std::wstring &utf16) {
        auto size = wcstombs_l(nullptr, utf16.c_str(), 0, l);
        if (!size) {
            return "";
        }
        std::string ret(size, 0);
        wcstombs_l(ret.data(), utf16.c_str(), size, l);
        return ret;
    }

    locale_t l;
};
#endif

std::wstring ToStdWString(const std::string &utf8) {
    if (utf8.empty()) {
        return L"";
    }
#ifdef _WIN32
    auto len = utf8.length() + 1;
    std::wstring ret(len, 0);
    auto size = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, &utf8[0], utf8.size(), &ret[0], len);
    ret.resize(size);
    return ret;
#else
#if 0
    size_t size = 0;
    _locale_t lc = _create_locale(LC_ALL, "en_US.UTF-8");
    errno_t retval = _mbstowcs_s_l(&size, &ret[0], len, &str[0], _TRUNCATE, lc);
    _free_locale(lc);
    ret.resize(size - 1);
    return ret;
#endif
    static LocalHelper local_helper;
    return local_helper.ToStdWString(utf8);
#endif
}

std::string ToUtf8String(const std::wstring &utf16) {
    if (utf16.empty()) {
        return "";
    }
#ifdef _WIN32
	const UINT UTF8_CODE_PAGE = 65001;
    auto size = ::WideCharToMultiByte(UTF8_CODE_PAGE, WC_ERR_INVALID_CHARS, &utf16[0], utf16.size(), nullptr, 0, nullptr, nullptr);
    std::string ret(size, 0);
	::WideCharToMultiByte(UTF8_CODE_PAGE, WC_ERR_INVALID_CHARS, &utf16[0], utf16.size(), &ret[0], size, nullptr, nullptr);
    return ret;
#else
#if 0
    size_t size = 0;
    _locale_t lc = _create_locale(LC_ALL, "en_US.UTF-8");
    errno_t err = _wcstombs_s_l(&size, NULL, 0, &wstr[0], _TRUNCATE, lc);
    std::string ret = std::string(size, 0);
    err = _wcstombs_s_l(&size, &ret[0], size, &wstr[0], _TRUNCATE, lc);
    _free_locale(lc);
    ret.resize(size - 1);
    return ret;
#endif
    static LocalHelper local_helper;
    return local_helper.ToUtf8String(utf16);
#endif
}

}
