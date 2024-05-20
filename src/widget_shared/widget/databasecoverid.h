//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <optional>
#include <utility>

#include <widget/database.h>

/*
 * Database cover id.
 * 
 * First is music id, second is album id.
*/
struct XAMP_WIDGET_SHARED_EXPORT DatabaseCoverId : public std::pair<int32_t, std::optional<int32_t>> {
	explicit DatabaseCoverId(int32_t music_id = kInvalidDatabaseId, std::optional<int32_t> album_id = std::nullopt) {
		first = music_id;
		second = album_id;
	}

	[[nodiscard]] bool isAlbumIdValid() const {
		return second.has_value();
	}

	[[nodiscard]] int32_t get() const {
		if (second) {
			return second.value();
		}
		return first;
	}

	bool operator<(const DatabaseCoverId& other) const {
		if (first != other.first) {
			return first < other.first;
		}
		const auto left = second ? second.value() : 0;
		const auto right = other.second ? other.second.value() : 0;
		if (left != right) {
			return left < right;
		}
		return false;
	}
};

Q_DECLARE_METATYPE(DatabaseCoverId)