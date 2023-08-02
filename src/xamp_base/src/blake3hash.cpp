#include <sstream>
#include <base/blake3hash.h>

#include <blake3.h>

XAMP_BASE_NAMESPACE_BEGIN

class Blake3Hash::Blake3HashImpl {
public:
	Blake3HashImpl() {
		Init();
	}

	void Init() {
		::blake3_hasher_init(&context_);
	}

	void Update(const char* bytes, size_t size) noexcept {
		::blake3_hasher_update(&context_, bytes, size);
	}

	void Finalize(uint8_t* buffer, size_t size) noexcept {
		::blake3_hasher_finalize(&context_, buffer, size);
	}

private:
	blake3_hasher context_{};
	
};

Blake3Hash::Blake3Hash()
	: impl_(MakeAlign<Blake3HashImpl>()) {
}

XAMP_PIMPL_IMPL(Blake3Hash)

void Blake3Hash::Update(const char* bytes, size_t size) noexcept {
	impl_->Update(bytes, size);
}

void Blake3Hash::Finalize() noexcept {
}

void Blake3Hash::GetHash(uint8_t* buffer, size_t size) noexcept {
	impl_->Finalize(buffer, size);
}

std::string Blake3Hash::GetHashString() {
	std::ostringstream ostr;
	ostr << GetHash();
	return ostr.str();
}

XAMP_BASE_NAMESPACE_END