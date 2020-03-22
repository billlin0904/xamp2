#include <cstdlib>
#include <base/hash.h>

namespace xamp::base {

#if !defined(_MSC_VER)
XAMP_ALWAYS_INLINE uint32_t _rotl(uint32_t x, int8_t r) noexcept {
	return (x << r) | (x >> (32 - r));
}

XAMP_ALWAYS_INLINE uint64_t _rotl64(uint64_t x, int8_t r) noexcept {
	return (x << r) | (x >> (64 - r));
}
#endif

#define ROTL32(x,y)	_rotl(x,y)
#define ROTL64(x,y)	_rotl64(x,y)

#define BIG_CONSTANT(x) (x##LLU)

XAMP_ALWAYS_INLINE uint32_t getblock(const uint32_t* p, int i) noexcept {
	return p[i];
}

XAMP_ALWAYS_INLINE uint64_t getblock(const uint64_t* p, int i) noexcept {
	return p[i];
}

XAMP_ALWAYS_INLINE uint32_t fmix(uint32_t h) noexcept {
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}

XAMP_ALWAYS_INLINE uint64_t fmix(uint64_t k) noexcept {
	k ^= k >> 33;
	k *= BIG_CONSTANT(0xff51afd7ed558ccd);
	k ^= k >> 33;
	k *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
	k ^= k >> 33;
	return k;
}

void MurmurHash3_x86_32(const void* key, int len, uint32_t seed, void* out) noexcept {
	const uint8_t* data = (const uint8_t*)key;
	const int nblocks = len / 4;

	uint32_t h1 = seed;

	uint32_t c1 = 0xcc9e2d51;
	uint32_t c2 = 0x1b873593;

	//----------
	// body

	const uint32_t* blocks = (const uint32_t*)(data + nblocks * 4);

	for (int i = -nblocks; i; i++)
	{
		uint32_t k1 = getblock(blocks, i);

		k1 *= c1;
		k1 = ROTL32(k1, 15);
		k1 *= c2;

		h1 ^= k1;
		h1 = ROTL32(h1, 13);
		h1 = h1 * 5 + 0xe6546b64;
	}

	//----------
	// tail

	const uint8_t* tail = (const uint8_t*)(data + nblocks * 4);

	uint32_t k1 = 0;

	switch (len & 3)
	{
	case 3: k1 ^= tail[2] << 16;
	case 2: k1 ^= tail[1] << 8;
	case 1: k1 ^= tail[0];
		k1 *= c1; k1 = ROTL32(k1, 15); k1 *= c2; h1 ^= k1;
	};

	//----------
	// finalization

	h1 ^= len;

	h1 = fmix(h1);

	*(uint32_t*)out = h1;
}

}