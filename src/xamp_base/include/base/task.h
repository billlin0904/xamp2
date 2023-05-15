//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <future>
#include <base/base.h>

XAMP_BASE_NAMESPACE_BEGIN

template <typename T = void>
using Task = std::future<T>;

template <typename T = void>
using SharedTask = std::shared_future<T>;

XAMP_BASE_NAMESPACE_END
