//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/ithreadpool.h>
#include <base/align_ptr.h>
#include <base/blocking_queue.h>

namespace xamp::base {

using SharedTaskQueue = BlockingQueue<Task>;
using SharedTaskQueuePtr = AlignPtr<SharedTaskQueue>;

using WorkStealingTaskQueue = BlockingQueue<Task>;
using WorkStealingTaskQueuePtr = AlignPtr<WorkStealingTaskQueue>;

}
