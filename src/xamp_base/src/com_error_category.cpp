#include <base/exception.h>
#include <base/com_error_category.h>
#include <base/dll.h>

#ifdef XAMP_OS_WIN
#include <Audioclient.h>
#include <comdef.h>
#include <comutil.h>

using namespace xamp::base;

namespace {
#ifdef XAMP_OS_WIN
	SharedLibraryHandle LoadSharedLibraryResource(const std::string_view& file_name) {
        return SharedLibraryHandle { ::LoadLibraryExA(file_name.data(),
            nullptr, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE) };
    }

    std::string GetModuleErrorMessage(const std::string_view& file_name, int32_t error) {
        constexpr size_t kMaxBufferSize = 256;
        std::string error_text;
        HANDLE locale_handle = nullptr;

        constexpr DWORD locale_system = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
        constexpr DWORD flags = FORMAT_MESSAGE_IGNORE_INSERTS |
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_HMODULE;
        
        auto dll = LoadSharedLibraryResource(file_name);
        ::FormatMessageA(
            flags,
            dll.get(),
            error,
            locale_system,
            reinterpret_cast<LPSTR>(&locale_handle),
            kMaxBufferSize,
            nullptr
        );

        if (locale_handle != nullptr) {
            auto ptr = ::LocalLock(locale_handle);
            if (ptr != nullptr) {
                error_text = static_cast<const char*>(ptr);
                ::LocalFree(locale_handle);
			}                       
        }
        return error_text;
    }
#endif
    std::string GetAudioClientErrors(HRESULT error) {
        static const std::vector<std::string> messages {
          "The IAudioClient object is not initialized.",
          "The IAudioClient object is already initialized.",
          "The endpoint device is a capture device, not a rendering device.",
          "The audio device has been unplugged or otherwise made unavailable.",
          "The audio stream was not stopped at the time of the Start call.",
          "The buffer is too large.",
          "A previous GetBuffer call is still in effect.",
          "The specified audio format is not supported.",
          "The wrong NumFramesWritten value.",
          "The endpoint device is already in use.",
          "Buffer operation pending",
          "The thread is not registered.",
          "The session spans more than one process.",
          "Exclusive mode is disabled on the device.",
          "Failed to create the audio endpoint.",
          "The Windows audio service is not running.",
          "The audio stream was not initialized for event-driven buffering.",
          "Exclusive mode only.",
          "The hnsBufferDuration and hnsPeriodicity parameters are not equal.",
          "Event handle not set.",
          "Incorrect buffer size.",
          "Buffer size error.",
          "CPU usage exceeded.",
          "Buffer error",
          "Buffer size not aligned."
        };

        auto index = error - AUDCLNT_E_NOT_INITIALIZED;
        if (index > messages.size()) {
	        return _com_error{ error }.ErrorMessage();
        }
        return messages[index];
	}

	std::unique_ptr<char[]> to_narrow(BSTR msg) {
        return std::unique_ptr<char[]>{
            _com_util::ConvertBSTRToString(msg)
        };
    }

    std::unique_ptr<char[]> to_narrow(const wchar_t* msg) {
        static_assert(std::is_same_v<wchar_t*, BSTR>);
        // const_cast is fine:
        // BSTR is a wchar_t*;
        // ConvertBSTRToString internally uses _wcslen and WideCharToMultiByte;
        return to_narrow(const_cast<wchar_t*>(msg));
    }
}

std::error_code make_error_code(com_error_enum e) noexcept {
    return { static_cast<int>(e), com_category() };
}

const std::error_category& com_category() noexcept {
    // immortalized object would be perfect, but isn¡¦t subject of this post
    static com_error_category ecat;
    return ecat;
}

std::string com_error_category::message(int hr) const {
    // leverage _com_error::ErrorMessage
#ifdef _UNICODE
    auto narrow = detail::to_narrow(_com_error{ hr }.ErrorMessage());
    return narrow.get();
#else
    return GetAudioClientErrors(hr);
#endif
}

std::error_condition com_error_category::default_error_condition(int hr) const noexcept {
    if (HRESULT_CODE(hr) || hr == 0)
        // system error condition
        return std::system_category().default_error_condition(HRESULT_CODE(hr));
    else
        // special error condition
        return { hr, com_category() };
}

std::system_error com_to_system_error(HRESULT hr, const std::string& msg) {
    // simply forward to com_to_system_error taking a C string
    return com_to_system_error(hr, msg.c_str());
}

std::system_error com_to_system_error(HRESULT hr, const char* msg) {
    // construct from error_code and message
    return { { static_cast<com_error_enum>(hr) }, msg };
}

std::system_error com_to_system_error(HRESULT hr, const wchar_t* msg) {
    return com_to_system_error(hr, *msg ? to_narrow(msg).get() : "");
}

std::system_error com_to_system_error(HRESULT hr, const std::wstring& msg) {
    return com_to_system_error(hr, msg.c_str());
}

std::system_error com_to_system_error(const _com_error& e) {
    // note: by forwarding to com_to_system_error(HRESULT, IErrorInfo*)
    // we benefit from hoisting _com_error::Description because _bstr_t wraps
    // up both unicode/ascii strings in a ref counter allocated on the heap
    return com_to_system_error(e.Error(), IErrorInfoPtr{ e.ErrorInfo(), false });
}

std::system_error com_to_system_error(HRESULT hr, IErrorInfo* help) {
    _com_error e(hr);

    using com_cstr = std::unique_ptr<OLECHAR[], decltype(::SysFreeString)*>;

    auto getDescription = [](IErrorInfo* help) -> com_cstr {
            BSTR description = nullptr;
            if (help)
                help->GetDescription(&description);
            return { description, &::SysFreeString };
        };

    com_cstr&& description = getDescription(help);
    // remove trailing newline or dot (end of last sentence)
    if (unsigned int length = description ? ::SysStringLen(description.get()) : 0) {
        unsigned int n = length;
        // place sentinel, other than [\r\n.]
        OLECHAR ch0 = std::exchange(description[0], L'\0');
        for (;;) {
            switch (description[n - 1]) {
            case L'\r':
            case L'\n':
            case L'.':
                continue;
			default: ;
            }
            break;
        }
        // note: null-terminating is less ideal than just finding the new EOS,
        // but system_error copies its description string argument,
        // hence we don't gain anything from range-construction
        if (n < length && n)
            description[n] = L'\0';
        // reestablish 1st character
        if (n)
            description[0] = ch0;

        return com_to_system_error(e.Error(), description.get());
    }

    // no description available
    return com_to_system_error(e.Error());
}

void WINAPI throw_translated_com_error(HRESULT hr, IErrorInfo* help) {
    throw com_to_system_error(hr, help);
}

#endif
