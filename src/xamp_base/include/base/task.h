//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <functional>
#include <future>
#include <stop_token>

#include <base/base.h>

XAMP_BASE_NAMESPACE_BEGIN

template <typename t = void>
using Future = std::future<t>;

template <typename t = void>
using SharedFuture = std::shared_future<t>;

using Task = std::move_only_function<void(const std::stop_token&)>;

XAMP_BASE_NAMESPACE_END
