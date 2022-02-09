#include <iomanip>
#include <sstream>

#include <base/exception.h>
#include <base/uuid.h>

namespace xamp::base {

static uint8_t MakeHex(char a, char b) {
    if (!std::isxdigit(a) || !std::isxdigit(b)) {
        throw std::invalid_argument("Invalid digital.");
    }
    const char buffer[3] = { a, b, '\0' };
    return static_cast<uint8_t>(std::strtoul(&buffer[0], nullptr, 16));
}

static UuidBuffer ParseUuid(std::string_view const & from_string) {
    if (from_string.length() != kMaxIdStrLen) {
        throw std::invalid_argument("Invalid Uuid.");
    }

    if (from_string[8] != '-'
        || from_string[13] != '-'
        || from_string[18] != '-'
        || from_string[23] != '-') {
        throw std::invalid_argument("Invalid Uuid.");
    }

    UuidBuffer uuid{};

    for (size_t i = 0, j = 0; i < from_string.length(); ++i) {
		if (from_string[i] == '-')
			continue;
		uuid[j] = MakeHex(from_string[i], from_string[i + 1]);
		++j;
		++i;
	}
    return uuid;
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

Uuid::Uuid(const uint8_t(&arr)[kIdSize]) noexcept {
    std::copy(std::cbegin(arr),
        std::cend(arr),
        std::begin(bytes_));
}

Uuid::Uuid(UuidBuffer const& bytes) noexcept
    : bytes_(bytes) {
}

Uuid::Uuid(std::string_view const &str) {
    bytes_ = ParseUuid(str);
}

Uuid::operator std::string() const {
    std::ostringstream ostr;
	ostr << *this;
    return ostr.str();
}

Uuid Uuid::FromString(std::string const & str) {
    return { str };
}
	
}
