//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <thread>
#include <vector>

#include <base/enum.h>
#include <base/base.h>
#include <base/jthread.h>

namespace xamp::base {

MAKE_XAMP_ENUM(
    ThreadPriority,
    BACKGROUND,
    NORMAL,
    HIGHEST)

MAKE_XAMP_ENUM(
    TaskSchedulerPolicy,
    ROUND_ROBIN_POLICY,
    RANDOM_POLICY,
    LEAST_LOAD_POLICY)

MAKE_XAMP_ENUM(
    TaskStealPolicy,
    CHILD_STEALING_POLICY,
    CONTINUATION_STEALING_POLICY)

struct XAMP_BASE_API PlatformUUID {
    uint8_t id[16];
};

inline constexpr auto XAMP_MAX_CPU = 256u;
inline constexpr auto XAMP_MASK_STRIDE = 64u;
inline constexpr auto XAMP_CPU_MASK_ROWS = (XAMP_MAX_CPU / XAMP_MASK_STRIDE);

struct XAMP_BASE_API CpuAffinity {
    void Clear(int32_t cpu) {
        auto [row, offset] = GetCPURowOffset(cpu);
        mask[row] &= ~(1ULL << offset);
    }

    void Set(int32_t cpu) {
        auto [row, offset] = GetCPURowOffset(cpu);
        mask[row] |= (1ULL << offset);
    }

    bool IsSet(int32_t cpu) const {
        auto [row, offset] = GetCPURowOffset(cpu);
        return (mask[row] & (1ULL << offset)) != 0;
    }

    int32_t FirstSetCpu() const;

    static std::tuple<uint32_t, uint32_t> GetCPURowOffset(int32_t cpu) {
        auto row = cpu / XAMP_MASK_STRIDE;
        return { row, cpu << XAMP_MASK_STRIDE * row };
    }

    friend bool operator != (const CpuAffinity& a, const CpuAffinity& b) {
        for (auto i = 0u; i < XAMP_CPU_MASK_ROWS; i++) {
            if (a.mask[i] != b.mask[i]) {
                return true;
            }
        }
        return false;
    }

    friend std::ostream& operator << (std::ostream &ostr, const CpuAffinity &aff) {
        for (auto i = 0u; i < XAMP_CPU_MASK_ROWS; i++) {
            ostr << "cpu mask[" << i << "]=" << aff.mask[i] << " ";
        }
        return ostr;
    }

    uint64_t mask[XAMP_CPU_MASK_ROWS]{0};
};

#ifdef XAMP_OS_WIN
struct XAMP_BASE_API ProcessorInformation {
    bool is_hyper_threaded{ false };

    struct XAMP_BASE_API Processor {
        uint32_t cpu_id{ 0 };
        uint32_t core_index{ 0 };
    };

    std::vector<Processor> processors;

    friend std::ostream& operator << (std::ostream& ostr, const ProcessorInformation& info) {
        ostr << "is_hyper_threaded:" << info.is_hyper_threaded;
        for (const auto processor : info.processors) {
            ostr << "cpu id[" << processor.cpu_id << "]=" << processor.core_index << " ";
        }
        return ostr;
    }
};
#endif

/// <summary>
/// Default thread pool affinity core.
/// </summary>
inline constexpr CpuAffinity kDefaultAffinityCpuCore{ 1 };

inline constexpr uint32_t kInfinity = -1;

XAMP_BASE_API void SetThreadPriority(JThread& thread, ThreadPriority priority) noexcept;

XAMP_BASE_API void SetThreadName(std::wstring const & name) noexcept;

XAMP_BASE_API void SetThreadAffinity(JThread& thread, CpuAffinity affinity = kDefaultAffinityCpuCore) noexcept;

XAMP_BASE_API std::string GetCurrentThreadId();

XAMP_BASE_API std::string MakeUuidString();

XAMP_BASE_API PlatformUUID ParseUuidString(const std::string& str);

XAMP_BASE_API bool IsDebuging();

XAMP_BASE_API bool VirtualMemoryLock(void* address, size_t size);

XAMP_BASE_API bool VirtualMemoryUnLock(void* address, size_t size);

XAMP_BASE_API void MSleep(std::chrono::milliseconds timeout);

XAMP_BASE_API int32_t AtomicWait(std::atomic<uint32_t>& to_wait_on, uint32_t expected, const timespec* to) noexcept;

XAMP_BASE_API bool AtomicWait(std::atomic<uint32_t>& to_wait_on, uint32_t &expected, uint32_t milliseconds) noexcept;

XAMP_BASE_API void AtomicWakeSingle(std::atomic<uint32_t>& to_wake) noexcept;

XAMP_BASE_API void AtomicWakeAll(std::atomic<uint32_t>& to_wake) noexcept;

XAMP_BASE_API uint64_t GenRandomSeed() noexcept;

XAMP_BASE_API void CpuRelax() noexcept;

XAMP_BASE_API void Assert(const char* message, const char* file, uint32_t line);

#ifdef XAMP_OS_WIN
XAMP_BASE_API void RedirectStdOut();
XAMP_BASE_API bool ExtendProcessWorkingSetSize(size_t size) noexcept;
XAMP_BASE_API bool SetProcessWorkingSetSize(size_t working_set_size) noexcept;
#endif
}
