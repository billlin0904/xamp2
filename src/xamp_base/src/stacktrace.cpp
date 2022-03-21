#include <cstdio>
#include <csignal>
#include <vector>
#include <optional>
#include <sstream>
#include <filesystem>

#include <base/logger.h>
#include <base/stl.h>
#include <base/singleton.h>
#include <base/stacktrace.h>
#include <base/str_utilts.h>
#include <base/align_ptr.h>

#ifdef XAMP_OS_WIN
#include <base/windows_handle.h>
#include <dbghelp.h>
#else
#include <cxxabi.h>
#include <execinfo.h>
#endif

namespace xamp::base {

#ifdef XAMP_OS_WIN

static std::string GetFileName(std::filesystem::path const& path) {
    return String::ToUtf8String(path.filename());
}

class SymLoader {
public:
    static constexpr auto kMaxModuleNameSize = 64;
    static constexpr auto kMaxSymbolNameSize = 255;

	[[nodiscard]] const WinHandle & GetProcess() const noexcept {
		return process_;
	}

    SymLoader() {
        symbol_.resize(sizeof(SYMBOL_INFO) + sizeof(wchar_t) * MAX_SYM_NAME);
        process_.reset(::GetCurrentProcess());
        thread_.reset(::GetCurrentThread());

        ::SymSetOptions(SYMOPT_DEFERRED_LOADS |
            SYMOPT_UNDNAME |
            SYMOPT_LOAD_LINES /*| SYMOPT_DEBUG*/);

        init_state_ = ::SymInitialize(process_.get(),
            nullptr, 
            TRUE);

        if (init_state_) {
            wchar_t path[MAX_PATH] = { 0 };
            ::GetModuleFileNameW(nullptr, path, MAX_PATH);
            Path excute_file_path(path);
            auto parent_path = excute_file_path.parent_path();
            ::SymSetSearchPathW(process_.get(), parent_path.c_str());
        }
    }

    [[nodiscard]] bool IsInit() const noexcept {
        return init_state_;
    }

	~SymLoader() {
        ::SymCleanup(process_.get());
	}

    size_t WalkStack(CONTEXT const* context, CaptureStackAddress& addrlist) noexcept {
        addrlist.fill(nullptr);

        STACKFRAME64 stack_frame{};

        stack_frame.AddrPC.Offset = context->Rip;
        stack_frame.AddrPC.Mode = AddrModeFlat;
        stack_frame.AddrStack.Offset = context->Rsp;
        stack_frame.AddrStack.Mode = AddrModeFlat;
        stack_frame.AddrFrame.Offset = context->Rbp;
        stack_frame.AddrFrame.Mode = AddrModeFlat;

        size_t frame_count = 0;

        for (auto& address : addrlist) {
            const auto result = ::StackWalk64(IMAGE_FILE_MACHINE_IA64,
                process_.get(),
                thread_.get(),
                &stack_frame,
                &context,
                nullptr,
                ::SymFunctionTableAccess64,
                ::SymGetModuleBase64,
                nullptr);
            if (!result) {
                address = nullptr;
                break;
            }
            if (stack_frame.AddrFrame.Offset == 0) {
                break;
            }
            address = reinterpret_cast<void*>(stack_frame.AddrPC.Offset);
            ++frame_count;
        }
        return frame_count;
    }

    void WriteLog(std::ostringstream &ostr, size_t frame_count, CaptureStackAddress& addrlist) {
        ostr << "\r\nstack backtrace:\r\n";

        frame_count = (std::min)(addrlist.size(), frame_count);

        for (size_t i = 0; i < frame_count; ++i) {
            auto* frame = addrlist[i];

            IMAGEHLP_MODULE64 module{};
            module.SizeOfStruct = sizeof(module);
            std::string module_name;

            ostr << "#" << std::left << std::setfill(' ') << std::setw(2) << std::dec << i << " ";

            const auto has_module = ::SymGetModuleInfo64(process_.get(),
                reinterpret_cast<DWORD64>(frame), 
                &module);
            if (has_module) {
                module_name = GetFileName(module.LoadedImageName);
                if (module_name.empty()) {
                    module_name = GetFileName(module.ImageName);
                }
            } else {
                module_name = "Unknown";
            }

            ostr << std::left << std::setfill(' ') << std::setw(24) << module_name << " ";

            MemorySet(symbol_.data(), 0, symbol_.size());
            auto* const symbol_info = reinterpret_cast<SYMBOL_INFO*>(symbol_.data());

            symbol_info->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol_info->MaxNameLen = kMaxSymbolNameSize;

            DWORD64 displacement = 0;
            const auto has_symbol = ::SymFromAddr(process_.get(),
                reinterpret_cast<DWORD64>(frame),
                &displacement,
                symbol_info);
            //const auto has_symbol = false;

            ostr << " \t"
                 << "0X" << std::hex << std::uppercase << std::setfill('0') << std::setw(8)
                 << reinterpret_cast<DWORD64>(frame) << " ";

            if (has_symbol) {
                DWORD line_displacement = 0;

                IMAGEHLP_LINE64 line{};
                line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

                const auto has_line = ::SymGetLineFromAddr64(process_.get(),
                    reinterpret_cast<DWORD64>(frame),
                    &line_displacement,
                    &line);

                ostr << symbol_info->Name;
                if (has_line) {
                    ostr << " in " << GetFileName(line.FileName) << ":" << std::dec << line.LineNumber << "\r\n";
                }
                else {
                    ostr << " + " << std::dec << displacement << "\r\n";
                }
            }
            else {
                ostr << "<unknown>" << "\r\n";
            }
        }
    }

private:
    bool init_state_;
	WinHandle process_;
    WinHandle thread_;
    std::vector<uint8_t> symbol_;
};

#define SYMBOL_LOADER Singleton<SymLoader>::GetInstance()
#endif

StackTrace::StackTrace() noexcept = default;

bool StackTrace::LoadSymbol() {
#ifdef XAMP_OS_WIN
    return SYMBOL_LOADER.IsInit();
#else
    return true;
#endif
}

std::string StackTrace::CaptureStack(const void* context) {
    CaptureStackAddress addrlist;
#ifdef XAMP_OS_WIN
    addrlist.fill(nullptr);
    std::ostringstream ostr;
    const auto frame_count = SYMBOL_LOADER.WalkStack(static_cast<const CONTEXT*>(context), addrlist);
    SYMBOL_LOADER.WriteLog(ostr, frame_count - 1, addrlist);
    return ostr.str();
#endif
    return "";
}

std::string StackTrace::CaptureStack() {
    CaptureStackAddress addrlist;
#ifdef XAMP_OS_WIN
    addrlist.fill(nullptr);
    std::ostringstream ostr;
    const auto frame_count = ::CaptureStackBackTrace(0, kMaxStackFrameSize, addrlist.data(), nullptr);
    SYMBOL_LOADER.WriteLog(ostr, frame_count - 1, addrlist);
    return ostr.str();
#else
    auto ParseSymbol = [](const std::string &symbol) -> std::vector<std::string> {
        std::istringstream iss(symbol);
        // 0: Index
        // 1: Module
        // 2: Address
        // 3: Function
        // 4: +
        // 5: Offset
        return std::vector<std::string>{
            std::istream_iterator<std::string>{iss},
            std::istream_iterator<std::string>{}
        };
    };
    auto addrlen = ::backtrace(addrlist.data(), static_cast<int32_t>(addrlist.size()));
    if (addrlen == 0) {
        return "";
    }

    std::ostringstream ostr;
    ostr << "\r\nstack backtrace:\r\n";

    auto symbollist = ::backtrace_symbols(addrlist.data(), addrlen);
    for (auto i = 0; i < addrlen; i++) {
        const auto strs = ParseSymbol(symbollist[i]);
        if (strs.size() != 6) {
            continue;
        }
        int status = 0;
        const CharPtr demangled(__cxxabiv1::__cxa_demangle(strs[3].c_str(), nullptr, nullptr, &status));
        std::string symbol;
        if (!demangled) {
            symbol = "<unknown>";
        } else {
            symbol = demangled.get();
        }
        ostr << std::left << std::setfill(' ') << std::setw(30) << strs[1] << " ";
        ostr << "at\t"
             << strs[2] << " ";
        ostr << " " << symbol << "\r\n";
    }
    return ostr.str();
#endif
}

}
