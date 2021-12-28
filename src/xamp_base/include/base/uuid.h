//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>
#include <string>
#include <ostream>
#include <array>

#include <base/base.h>

namespace xamp::base {
	
inline constexpr size_t kIdSize = 16;
inline constexpr size_t kMaxIdStrLen = 36;
using UuidBuffer = std::array<uint8_t, kIdSize>;

class XAMP_BASE_API Uuid final {
public:
    static Uuid const kInvalidUUID;

    static Uuid FromString(std::string const & str);

    template <typename ForwardIterator>
    explicit Uuid(ForwardIterator first, ForwardIterator last) {
        if (std::distance(first, last) == kIdSize) {
            std::copy(first, last, std::begin(bytes_));
        }
    }

    explicit Uuid(uint8_t(&arr)[16]) noexcept {
        std::copy(std::cbegin(arr), std::cend(arr), std::begin(bytes_));
    }

    Uuid() noexcept;

    explicit Uuid(UuidBuffer const &bytes) noexcept;

    Uuid(std::string_view const &from_string);

	[[nodiscard]] bool IsValid() const noexcept;

    Uuid(Uuid const &other) noexcept;
    
    Uuid& operator=(Uuid const &other) noexcept;

    Uuid(Uuid &&other) noexcept;
    
    Uuid& operator=(Uuid &&other) noexcept;

	[[nodiscard]] size_t GetHash() const;

	operator std::string() const;

private:
    XAMP_BASE_API friend std::ostream &operator<<(std::ostream &s, Uuid const &id);

    XAMP_BASE_API friend bool operator==(std::string const &str, Uuid const &id);

    XAMP_BASE_API friend bool operator==(Uuid const & other1, Uuid const & other2) noexcept;

    XAMP_BASE_API friend bool operator!=(Uuid const & other1, Uuid const & other2) noexcept;

    [[nodiscard]] size_t CalcHash() const noexcept;

    size_t hash_;
    UuidBuffer bytes_{0};
};

XAMP_ALWAYS_INLINE bool Uuid::IsValid() const noexcept {
    return *this == Uuid::kInvalidUUID;
}

XAMP_ALWAYS_INLINE bool operator!=(Uuid const & other1, Uuid const & other2) noexcept {
    return !(std::memcmp(other1.bytes_.data(), other2.bytes_.data(), other2.bytes_.size()) == 0);
}

XAMP_ALWAYS_INLINE bool operator==(Uuid const & other1, Uuid const & other2) noexcept {
    return std::memcmp(other1.bytes_.data(), other2.bytes_.data(), other2.bytes_.size()) == 0;
}

XAMP_ALWAYS_INLINE bool operator==(std::string const &str, Uuid const &id) {
	return Uuid(str) == id;
}

XAMP_ALWAYS_INLINE size_t Uuid::GetHash() const {
	return hash_;
}

}

namespace std {

template <>
struct hash<xamp::base::Uuid> {
	typedef size_t result_type;
	typedef xamp::base::Uuid argument_type;

	result_type operator()(argument_type const& uuid) const noexcept {
		return uuid.GetHash();
	}
};

}

