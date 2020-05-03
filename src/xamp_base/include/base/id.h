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
	
constexpr size_t XAMP_ID_MAX_SIZE = 16;
constexpr size_t MAX_ID_STR_LEN = 36;

class XAMP_BASE_API ID final {
public:
	static const ID INVALID_ID;

    static ID FromString(const std::string & str);

    ID() noexcept;

	explicit ID(const std::array<uint8_t, XAMP_ID_MAX_SIZE> &bytes) noexcept;

	explicit ID(const std::string_view &from_string);

    bool IsValid() const noexcept;

    ID(const ID &other) noexcept;
    
    ID& operator=(const ID &other) noexcept;

    ID(ID &&other) noexcept;
    
    ID& operator=(ID &&other) noexcept;	

	size_t GetHash() const noexcept;

	operator std::string() const noexcept;

private:
	friend std::ostream &operator<<(std::ostream &s, const ID &id);

	friend bool operator==(const std::string &str, const ID &id) noexcept;

	friend bool operator==(const ID& other1, const ID& other2) noexcept;

	friend bool operator!=(const ID& other1, const ID& other2) noexcept;

	std::array<uint8_t, XAMP_ID_MAX_SIZE> bytes_;
};

XAMP_ALWAYS_INLINE ID::ID(const ID &other) noexcept {
	*this = other;
}

XAMP_ALWAYS_INLINE ID & ID::operator=(const ID & other) noexcept {
	if (this != &other) {
		bytes_ = other.bytes_;
	}
	return *this;
}

XAMP_ALWAYS_INLINE ID::ID(ID &&other) noexcept {
    *this = std::move(other);
}

XAMP_ALWAYS_INLINE ID& ID::operator=(ID&& other) noexcept {
	if (this != &other) {
		bytes_ = other.bytes_;
	}
	return *this;
}

XAMP_ALWAYS_INLINE bool ID::IsValid() const noexcept {
    return *this == ID::INVALID_ID;
}

XAMP_ALWAYS_INLINE bool operator!=(const ID & other1, const ID & other2) noexcept {
    return !(std::memcmp(other1.bytes_.data(), other2.bytes_.data(), other2.bytes_.size()) == 0);
}

XAMP_ALWAYS_INLINE bool operator==(const ID& other1, const ID& other2) noexcept {
    return std::memcmp(other1.bytes_.data(), other2.bytes_.data(), other2.bytes_.size()) == 0;
}

XAMP_ALWAYS_INLINE bool operator==(const std::string &str, const ID &id) noexcept {
	return ID(str) == id;
}

XAMP_ALWAYS_INLINE size_t ID::GetHash() const noexcept {
	return std::hash<std::string>{}(std::string());
}

}

namespace std {

template <>
struct hash<xamp::base::ID> {
	typedef size_t result_type;
	typedef xamp::base::ID argument_type;

	result_type operator()(const argument_type & uuid) const {
		return uuid.GetHash();
	}
};

}

