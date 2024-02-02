#include <base/stacktrace.h>

#include <base/fs.h>
#include <base/stl.h>
#include <base/singleton.h>
#include <base/str_utilts.h>
#include <base/memory.h>

#ifdef XAMP_OS_WIN
#include <base/platfrom_handle.h>
#include <dbghelp.h>
#else
#include <cxxabi.h>
#include <execinfo.h>
#endif

#include <cstdio>
#include <csignal>
#include <vector>
#include <optional>
#include <sstream>
#include <filesystem>

XAMP_BASE_NAMESPACE_BEGIN

#ifdef XAMP_OS_WIN

namespace {
    std::string GetFileName(std::filesystem::path const& path) {
        return String::ToUtf8String(path.filename());
    }

    struct StackTraceEntry {
        bool has_module{ false };
        bool has_symbol{ false };
        bool has_line{ false };
        uint32_t index{ 0 };
        uint32_t source_line{ 0 };
        uint64_t address{ 0 };
        uint64_t displacement{ 0 };
        std::string symbol_name;
        std::string module_name;
        std::string source_file_name;
    };

    inline constexpr auto kMaxModuleNameSize = 64;
    inline constexpr auto kMaxSymbolNameSize = 255;

    class SymLoader final {
    public:
        XAMP_DISABLE_COPY(SymLoader)

    	SymLoader() {
            process_.reset(::GetCurrentProcess());

            ::SymSetOptions(SYMOPT_DEFERRED_LOADS |
                SYMOPT_UNDNAME |
                SYMOPT_LOAD_LINES);

            init_state_ = ::SymInitialize(process_.get(),
                nullptr,
                TRUE);

            if (init_state_) {
                wchar_t path[MAX_PATH] = { 0 };
                ::GetModuleFileNameW(nullptr, path, MAX_PATH);
                const Path execute_file_path(path);
                const auto parent_path = execute_file_path.parent_path();
                ::SymSetSearchPathW(process_.get(), parent_path.c_str());
            }
        }

        [[nodiscard]] bool IsInit() const noexcept {
            return init_state_;
        }

        ~SymLoader() {
            ::SymCleanup(process_.get());
        }

        HANDLE GetCurrentProcess() const noexcept {
            return process_.get();
        }

    private:
        bool init_state_;
        WinHandle process_;
    };

#define SYMBOL_LOADER Singleton<SymLoader>::GetInstance()

    size_t WalkStack(CONTEXT const* context, CaptureStackAddress& addrlist) noexcept {
        addrlist.fill(nullptr);

        STACKFRAME64 stack_frame{};

        stack_frame.AddrPC.Offset = context->Rip;
        stack_frame.AddrPC.Mode = AddrModeFlat;
        stack_frame.AddrStack.Offset = context->Rsp;
        stack_frame.AddrStack.Mode = AddrModeFlat;
        stack_frame.AddrFrame.Offset = context->Rbp;
        stack_frame.AddrFrame.Mode = AddrModeFlat;

        WinHandle thread(::GetCurrentThread());
        size_t frame_count = 0;

        for (auto& address : addrlist) {
            const auto result = ::StackWalk64(IMAGE_FILE_MACHINE_IA64,
                SYMBOL_LOADER.GetCurrentProcess(),
                thread.get(),
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

    void WriteLog(std::ostringstream& ostr, size_t frame_count, CaptureStackAddress& addrlist) {
        ostr.str("");
        ostr.clear();

        ostr << "\r\nstack backtrace:\r\n";

        std::vector<uint8_t> symbol_buffer;
        symbol_buffer.resize(sizeof(SYMBOL_INFO) + sizeof(wchar_t) * MAX_SYM_NAME);

        std::vector<StackTraceEntry> stack_trace_entries;
        stack_trace_entries.reserve(kMaxStackFrameSize);

        frame_count = (std::min)(addrlist.size(), frame_count);
        size_t max_width = 0;

        for (uint32_t i = 0; i < frame_count; ++i) {
            StackTraceEntry entry;
            entry.index = i;
            entry.address = reinterpret_cast<uint64_t>(addrlist[i]);

            IMAGEHLP_MODULE64 module{};
            module.SizeOfStruct = sizeof(module);
            entry.has_module = ::SymGetModuleInfo64(SYMBOL_LOADER.GetCurrentProcess(),
                entry.address,
                &module);
            if (entry.has_module) {
                entry.module_name = GetFileName(module.LoadedImageName);
                if (entry.module_name.empty()) {
                    entry.module_name = GetFileName(module.ImageName);
                }
            }
            else {
                entry.module_name = "Unknown";
            }

            max_width = (std::max)(max_width, entry.module_name.length());

            MemorySet(symbol_buffer.data(), 0, symbol_buffer.size());
            auto* const symbol_info = reinterpret_cast<SYMBOL_INFO*>(symbol_buffer.data());
            symbol_info->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol_info->MaxNameLen = kMaxSymbolNameSize;

            DWORD64 displacement = 0;
            entry.has_symbol = ::SymFromAddr(GetCurrentProcess(),
                entry.address,
                &displacement,
                symbol_info);

            if (entry.has_symbol) {
                IMAGEHLP_LINE64 line{};
                line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

                DWORD line_displacement = 0;
                entry.has_line = ::SymGetLineFromAddr64(SYMBOL_LOADER.GetCurrentProcess(),
                    entry.address,
                    &line_displacement,
                    &line);

                entry.symbol_name = symbol_info->Name;

                if (entry.has_line) {
                    entry.source_file_name = GetFileName(line.FileName);
                    entry.source_line = line.LineNumber;
                }
                else {
                    entry.displacement = displacement;
                }
            }
            else {
                entry.source_file_name = "<unknown>";
            }

            stack_trace_entries.push_back(entry);
        }

        for (const auto& entry : stack_trace_entries) {
            ostr << "\t#" << std::left << std::setfill(' ') << std::setw(2) << std::dec << entry.index << " "
                << std::left << std::setfill(' ') << std::setw(max_width) << entry.module_name << " "
                << " "
                << std::left << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << entry.address << " ";

            if (entry.has_symbol) {
                ostr << entry.symbol_name;

                if (entry.has_line) {
                    ostr << " in " << entry.source_file_name << ":" << std::dec << entry.source_line << "\r\n";
                }
                else {
                    ostr << " + " << std::dec << entry.displacement << "\r\n";
                }
            }
            else {
                ostr << "<unknown>" << "\r\n";
            }
        }
    }
}

#endif

StackTrace::StackTrace() noexcept = default;

bool StackTrace::LoadSymbol() {
#ifdef XAMP_OS_WIN
    return SYMBOL_LOADER.IsInit();
#else
    return true;
#endif
}

std::string StackTrace::CaptureStack(uint32_t skip) {
    auto skip_frame = (std::min)(skip, 1U);
    CaptureStackAddress addrlist;
#ifdef XAMP_OS_WIN
    addrlist.fill(nullptr);
    std::ostringstream ostr;
    const auto frame_count = ::CaptureStackBackTrace(skip_frame, kMaxStackFrameSize, addrlist.data(), nullptr);
    WriteLog(ostr, frame_count - 1, addrlist);
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

XAMP_BASE_NAMESPACE_END
