
//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/str_utilts.h>

#include <tl/expected.hpp>

XAMP_BASE_NAMESPACE_BEGIN

template <typename T, typename E = std::string>
using Expected = tl::expected<T, E>;


template <typename... Args>
tl::unexpected<std::string> MakeUnexpected(std::string_view s, Args &&...args) {
	return tl::unexpected(String::Format(s, std::forward<Args>(args)...));
}


template <typename F, typename E>
auto expect(F&& func) noexcept -> Expected<std::invoke_result_t<F>, E> {
	try {
		return std::invoke(std::forward<F>(func));
	}
	catch (std::exception& ex) {
		return tl::unexpected(ex.what());
	}
}

XAMP_BASE_NAMESPACE_END

