//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/align_ptr.h>
#include <base/moveonly_function.h>
#include <base/blocking_queue.h>
#include <base/mpmc_queue.h>

XAMP_BASE_NAMESPACE_BEGIN

using SharedTaskQueue = BlockingQueue<MoveOnlyFunction>;
using SharedTaskQueuePtr = AlignPtr<SharedTaskQueue>;

using WorkStealingTaskQueue = MpmcQueue<MoveOnlyFunction>;
//using WorkStealingTaskQueue = BlockingQueue<MoveOnlyFunction>;
using WorkStealingTaskQueuePtr = AlignPtr<WorkStealingTaskQueue>;

XAMP_BASE_NAMESPACE_END
