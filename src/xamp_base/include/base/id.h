//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>
#include <string>
#include <ostream>
#include <array>
#include <string_view>
#include <ctime>

#include <base/base.h>

namespace xamp::base {
	
inline constexpr size_t kIdSize = 16;
inline constexpr size_t kMaxIdStrLen = 36;

class XAMP_BASE_API ID final {
public:
    static ID const INVALID_ID;

    static ID FromString(std::string const & str);

    ID() noexcept;

    explicit ID(std::array<uint8_t, kIdSize> const &bytes) noexcept;

    ID(std::string_view const &from_string);

	[[nodiscard]] bool IsValid() const noexcept;

    ID(ID const &other) noexcept;
    
    ID& operator=(ID const &other) noexcept;

    ID(ID &&other) noexcept;
    
    ID& operator=(ID &&other) noexcept;

	[[nodiscard]] size_t GetHash() const;

	operator std::string() const;

private:
    XAMP_BASE_API friend std::ostream &operator<<(std::ostream &s, ID const &id);

    XAMP_BASE_API friend bool operator==(std::string const &str, ID const &id);

    XAMP_BASE_API friend bool operator==(ID const & other1, ID const & other2) noexcept;

    XAMP_BASE_API friend bool operator!=(ID const & other1, ID const & other2) noexcept;

    size_t CalcHash() const noexcept;

    size_t hash_;
    std::array<uint8_t, kIdSize> bytes_;
};

XAMP_ALWAYS_INLINE bool ID::IsValid() const noexcept {
    return *this == ID::INVALID_ID;
}

XAMP_ALWAYS_INLINE bool operator!=(ID const & other1, ID const & other2) noexcept {
    return !(std::memcmp(other1.bytes_.data(), other2.bytes_.data(), other2.bytes_.size()) == 0);
}

XAMP_ALWAYS_INLINE bool operator==(ID const & other1, ID const & other2) noexcept {
    return std::memcmp(other1.bytes_.data(), other2.bytes_.data(), other2.bytes_.size()) == 0;
}

XAMP_ALWAYS_INLINE bool operator==(std::string const &str, ID const &id) {
	return ID(str) == id;
}

XAMP_ALWAYS_INLINE size_t ID::GetHash() const {
	return hash_;
}

}

namespace std {

template <>
struct hash<xamp::base::ID> {
	typedef size_t result_type;
	typedef xamp::base::ID argument_type;

	result_type operator()(argument_type const& uuid) const {
		return uuid.GetHash();
	}
};

}

