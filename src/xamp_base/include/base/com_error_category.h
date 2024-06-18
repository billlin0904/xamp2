#pragma once

#include <system_error>
#include <type_traits>

#include <base/platfrom_handle.h>

#ifdef XAMP_OS_WIN

// https://kb.firedaemon.com/support/solutions/articles/4000121648-fitting-com-into-c-system-error-handling

class _com_error;
struct IErrorInfo;

enum class com_error_enum {};

namespace std {
	template<>
	struct is_error_code_enum<com_error_enum> : true_type {
	};
}

XAMP_BASE_API std::error_code make_error_code(com_error_enum e) noexcept;

XAMP_BASE_API const std::error_category& com_category() noexcept;

class XAMP_BASE_API com_error_category : public std::error_category {
public:
	using error_category::error_category;

	const char* name() const noexcept override {
		return "com";
	}

	// @note If _UNICODE is defined the error description gets
	// converted to an ANSI string using the CP_ACP codepage.
	std::string message(int hr) const override;

	// Make error_condition for error code (generic if possible)
	// @return system's default error condition if error value can be mapped to a Windows error, error condition with com category otherwise
	std::error_condition default_error_condition(int hr) const noexcept override;
};

// Factory function creating a std::system_error from a HRESULT error code
// @param msg Description to prepend to the error code message; must not be nullptr.
XAMP_BASE_API std::system_error com_to_system_error(HRESULT code, const char* msg = "");

// Factory function creating a std::system_error from a HRESULT error code
// @param msg Description to prepend to the error code message.
XAMP_BASE_API std::system_error com_to_system_error(HRESULT code, const std::string& msg);

// Factory function creating a std::system_error from a HRESULT error code
// and an optional error message
// @param msg Optional description to prepend to the error code message; // must not be nullptr;
// gets converted to an ANSI string using the CP_ACP codepage.
XAMP_BASE_API std::system_error com_to_system_error(HRESULT hr, const wchar_t* msg);

// Factory function creating a std::system_error from a HRESULT error code
// and an optional error message
// @param msg Optional description to prepend to the error code message;
// gets converted to an ANSI string using the CP_ACP codepage.
XAMP_BASE_API std::system_error com_to_system_error(HRESULT hr, const std::wstring& msg);

// Factory function creating a std::system_error from a _com_error.
// If an error description is available, removes trailing newline or dot (end of last sentence) and prepends it to the error code message.
// @note The error description gets converted to an ANSI string using the CP_ACP codepage.
XAMP_BASE_API std::system_error com_to_system_error(const _com_error& e);

// Factory function creating a std::system_error from a HRESULT error code and optionally from an error information.
// If an error description is available, removes trailing newline or dot (end of last sentence) and prepends it to the error code message.
// @note The error description gets converted to an ANSI string using the CP_ACP codepage.
XAMP_BASE_API std::system_error com_to_system_error(HRESULT hr, IErrorInfo* help);

// Translate COM error and throw std::system_error.
// Suitable as alternative COM error handler, settable with _set_com_error_handler
// @note The error description gets converted to an ANSI string using the CP_ACP codepage.
XAMP_BASE_API void WINAPI throw_translated_com_error(HRESULT hr, IErrorInfo* help = nullptr);

#endif