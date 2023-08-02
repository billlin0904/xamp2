//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <array>
#include <iomanip>
#include <string>

#include <base/base.h>
#include <base/align_ptr.h>

XAMP_BASE_NAMESPACE_BEGIN

constexpr size_t kBlake3OutLen = 32;

template <size_t NumberBytes = kBlake3OutLen>
using Blake3Result = std::array<uint8_t, NumberBytes>;

class XAMP_BASE_API Blake3Hash {
public:
	Blake3Hash();

    XAMP_PIMPL(Blake3Hash)

    template <typename CharT>
    void Update(const std::basic_string< CharT>& str) noexcept {
        Update(reinterpret_cast<const char*>(str.data()), str.size() * sizeof(CharT));
    }

    template <typename CharT>
    void Update(const CharT* str, size_t length) noexcept {
        Update(reinterpret_cast<const char*>(str), length * sizeof(CharT));
    }

	void Finalize() noexcept;

    template <size_t NumberBytes = kBlake3OutLen>
    Blake3Result<NumberBytes> GetHash() noexcept {
        Blake3Result<NumberBytes> result;
        GetHash(result.data(), NumberBytes);
        return result;
    }

    size_t Get64bitHash() {
        static constexpr size_t kMax64bitBytes = 8;
        Blake3Result<kMax64bitBytes> result{};
        GetHash(result.data(), result.size());
        size_t hash = 0;
        for (size_t i = 0; i < kMax64bitBytes; ++i) {
            hash |= static_cast<uint64_t>(result[i]) << (8 * i);
        }
        return hash;
    }

    std::string GetHashString();

    template <typename CharT, size_t NumberBytes = kBlake3OutLen>
    static Blake3Result<NumberBytes> Hash(const std::basic_string<CharT>& str) {
        Blake3Hash hash;
        hash.Update(str);
        hash.Finalize();
        return hash.GetHash<NumberBytes>();
    }
private:
    XAMP_BASE_API friend std::ostream& operator<<(std::ostream& ostr, const Blake3Result<>& result);

    void Update(const char* bytes, size_t size) noexcept;

    void GetHash(uint8_t*buffer, size_t size) noexcept;

    class Blake3HashImpl;
    AlignPtr<Blake3HashImpl> impl_;
};

XAMP_ALWAYS_INLINE std::ostream& operator<<(std::ostream& ostr, const Blake3Result<>& result) {
    for (const auto elem : result) {
        ostr << std::setfill('0')
            << std::setw(2)
            << std::uppercase
            << std::hex
            << (0xFF & elem);
    }
    return ostr;
}

XAMP_BASE_NAMESPACE_END