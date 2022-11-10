//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/ithreadpool.h>
#include <base/align_ptr.h>
#include <base/blocking_queue.h>
#include <base/spinlock.h>

namespace xamp::base {

using SharedTaskQueue = BlockingQueue<MoveableFunction>;
using SharedTaskQueuePtr = AlignPtr<SharedTaskQueue>;

using WorkStealingTaskQueue = BlockingQueue<MoveableFunction>;
using WorkStealingTaskQueuePtr = AlignPtr<WorkStealingTaskQueue>;

}
