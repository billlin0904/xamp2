#pragma once

#include <memory>
#include <string_view>
#include <thread>

#include <base/base.h>
#include <base/platform.h>

XAMP_BASE_NAMESPACE_BEGIN

class IThreadPool;

struct XAMP_BASE_API ThreadPoolBuilder {
    static std::shared_ptr<IThreadPool> makeThreadPool(const std::string_view& pool_name,
        uint32_t max_thread = std::thread::hardware_concurrency(),
        size_t bulk_size = std::thread::hardware_concurrency() / 2,
        ThreadPriority priority = ThreadPriority::PRIORITY_NORMAL);

    static std::shared_ptr<IThreadPool> makeBackgroundThreadPool();

    static std::shared_ptr<IThreadPool> makePlaybackThreadPool();

    static std::shared_ptr<IThreadPool> makePlayerThreadPool();
};

XAMP_BASE_NAMESPACE_END
