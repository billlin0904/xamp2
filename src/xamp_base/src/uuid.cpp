#include <iomanip>
#include <sstream>
#include <charconv>
#include <base/uuid.h>

namespace xamp::base {

static bool TryParseHex(const char a, const char b, uint8_t &data) noexcept {
    const char buffer[] = { a, b, '\0' };
    uint32_t result = 0;
    auto [_, ec] = std::from_chars(std::cbegin(buffer), std::cend(buffer), result, 16);
    if (ec != std::errc{}) {
        return false;
    }
    data = static_cast<uint8_t>(result);
    return true;
}

static bool TryParseUuid(std::string_view const & hex_string, UuidBuffer &result) noexcept {
    if (hex_string.length() != kMaxUuidHexStringLength) {
        return false;
    }

    if (   hex_string[8]  != '-'
        || hex_string[13] != '-'
        || hex_string[18] != '-'
        || hex_string[23] != '-') {
        return false;
    }

    UuidBuffer uuid{};

    for (size_t i = 0, j = 0; i < hex_string.length(); ++i) {
        switch (i) {
        case 8:
        case 13:
        case 18:
        case 23:
            continue;
        default: 
            if (!TryParseHex(hex_string[i], hex_string[i + 1], uuid[j])) {
                return false;
			}
            ++j;
            ++i;
        }		
	}
    result = uuid;
    return true;
}

std::ostream &operator<<(std::ostream &s, Uuid const &id) {
  return s << std::hex << std::setfill('0')
    << std::setw(2) << static_cast<int32_t>(id.bytes_[0])
    << std::setw(2) << static_cast<int32_t>(id.bytes_[1])
    << std::setw(2) << static_cast<int32_t>(id.bytes_[2])
    << std::setw(2) << static_cast<int32_t>(id.bytes_[3])
    << "-"                                  
    << std::setw(2) << static_cast<int32_t>(id.bytes_[4])
    << std::setw(2) << static_cast<int32_t>(id.bytes_[5])
    << "-"                                  
    << std::setw(2) << static_cast<int32_t>(id.bytes_[6])
    << std::setw(2) << static_cast<int32_t>(id.bytes_[7])
    << "-"                                  
    << std::setw(2) << static_cast<int32_t>(id.bytes_[8])
    << std::setw(2) << static_cast<int32_t>(id.bytes_[9])
    << "-"                                  
    << std::setw(2) << static_cast<int32_t>(id.bytes_[10])
    << std::setw(2) << static_cast<int32_t>(id.bytes_[11])
    << std::setw(2) << static_cast<int32_t>(id.bytes_[12])
    << std::setw(2) << static_cast<int32_t>(id.bytes_[13])
    << std::setw(2) << static_cast<int32_t>(id.bytes_[14])
    << std::setw(2) << static_cast<int32_t>(id.bytes_[15]);
}

Uuid const Uuid::kNullUuid;

Uuid::Uuid() noexcept = default;

Uuid::Uuid(Uuid const& other) noexcept
	: Uuid() {
    *this = other;
}

Uuid& Uuid::operator=(Uuid const& other) noexcept {
    if (this != &other) {
        bytes_ = other.bytes_;
    }
    return *this;
}

Uuid::Uuid(Uuid&& other) noexcept
	: Uuid() {
    *this = std::move(other);
}

Uuid& Uuid::operator=(Uuid&& other) noexcept {
    if (this != &other) {
        bytes_ = other.bytes_;
        other.bytes_.fill(0);
    }
    return *this;
}

Uuid::Uuid(const uint8_t(&byte_array)[kIdSize]) noexcept {
    std::copy(std::cbegin(byte_array),
        std::cend(byte_array),
        std::begin(bytes_));
}

Uuid::Uuid(UuidBuffer const& buffer) noexcept
    : bytes_(buffer) {
}

Uuid::Uuid(std::string_view const &str) {
    TryParseUuid(str, bytes_);
}

Uuid::operator std::string() const {
    std::ostringstream ostr;
	ostr << *this;
    return ostr.str();
}

Uuid Uuid::FromString(std::string const & hex_string) {
    return { hex_string };
}

bool Uuid::TryParseString(std::string const& hex_string, Uuid& uuid) {
    UuidBuffer buffer;
    if (!TryParseUuid(hex_string, buffer)) {
        return false;
    }
	const Uuid result(buffer);
    if (!result.IsValid()) {
        return false;
    }
    uuid = result;
    return true;
}
	
}
