//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/enum.h>

MAKE_XAMP_ENUM(PlayerOrder,
		  PLAYER_ORDER_REPEAT_ONCE,
		  PLAYER_ORDER_REPEAT_ONE,
		  PLAYER_ORDER_SHUFFLE_ALL,
		  PLAYER_ORDER_MAX)

inline PlayerOrder GetNextOrder(PlayerOrder cur) noexcept {
    return static_cast<PlayerOrder>((static_cast<int32_t>(cur) + 1)
        % static_cast<uint32_t>(PlayerOrder::PLAYER_ORDER_MAX));
}