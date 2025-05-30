//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/memory.h>
#include <base/moveonly_function.h>
#include <base/blocking_queue.h>
#include <base/concurrentqueue.h>

XAMP_BASE_NAMESPACE_BEGIN

using SharedTaskQueue = BlockingQueue<MoveOnlyFunction>;
using SharedTaskQueuePtr = ScopedPtr<SharedTaskQueue>;

template <typename T>
using ConcurrentQueue = moodycamel::ConcurrentQueue<T>;

using WorkStealingTaskQueue = moodycamel::ConcurrentQueue<MoveOnlyFunction>;
using WorkStealingTaskQueuePtr = ScopedPtr<WorkStealingTaskQueue>;

XAMP_BASE_NAMESPACE_END
