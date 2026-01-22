//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/memory.h>
#include <base/task.h>
#include <base/blocking_queue.h>
#include <base/concurrentqueue.h>

XAMP_BASE_NAMESPACE_BEGIN

using SharedTaskQueue = BlockingQueue<Task>;
using SharedTaskQueuePtr = ScopedPtr<SharedTaskQueue>;

template <typename T>
using ConcurrentQueue = moodycamel::ConcurrentQueue<T>;

using WorkStealingTaskQueue = moodycamel::ConcurrentQueue<Task>;
using WorkStealingTaskQueuePtr = ScopedPtr<WorkStealingTaskQueue>;

XAMP_BASE_NAMESPACE_END
