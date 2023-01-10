//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/align_ptr.h>
#include <base/moveonly_function.h>
#include <base/blocking_queue.h>
#include <base/spmc_queue.h>

namespace xamp::base {

using SharedTaskQueue = BlockingQueue<MoveOnlyFunction>;
using SharedTaskQueuePtr = AlignPtr<SharedTaskQueue>;

using WorkStealingTaskQueue = BlockingQueue<MoveOnlyFunction>;
//using WorkStealingTaskQueue = SpmcQueue<MoveOnlyFunction>;
using WorkStealingTaskQueuePtr = AlignPtr<WorkStealingTaskQueue>;

}
