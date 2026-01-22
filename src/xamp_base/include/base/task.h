//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <future>
#include <base/base.h>

XAMP_BASE_NAMESPACE_BEGIN

template <typename T = void>
using Future = std::future<T>;

template <typename T = void>
using SharedFuture = std::shared_future<T>;

using Task = std::move_only_function<void(const std::stop_token&)>;

XAMP_BASE_NAMESPACE_END
