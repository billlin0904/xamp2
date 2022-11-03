#include <base/math.h>
#include <base/siphash.h>

namespace xamp::base {

#define SIPROUND                                                          \
    do                                                                    \
    {                                                                     \
        v0_ += v1_; v1_ = Rotl64(v1_, 13); v1_ ^= v0_; v0_ = Rotl64(v0_, 32); \
        v2_ += v3_; v3_ = Rotl64(v3_, 16); v3_ ^= v2_;                      \
        v0_ += v3_; v3_ = Rotl64(v3_, 21); v3_ ^= v0_;                      \
        v2_ += v1_; v1_ = Rotl64(v1_, 17); v1_ ^= v2_; v2_ = Rotl64(v2_, 32); \
    } while(0)

#if 0 // __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define CURRENT_BYTES_IDX(i) (7 - i)
#else
#define CURRENT_BYTES_IDX(i) (i)
#endif

 SipHash::SipHash(uint64_t k0, uint64_t k1) {
     v0_ = 0x736f6d6570736575ULL ^ k0;
     v1_ = 0x646f72616e646f6dULL ^ k1;
     v2_ = 0x6c7967656e657261ULL ^ k0;
     v3_ = 0x7465646279746573ULL ^ k1;

     count_ = 0;
     current_word = 0;
 }

 void SipHash::Update(const char* data, uint64_t size) {
     const char* end = data + size;

     /// We'll finish to process the remainder of the previous update, if any.
     if (count_ & 7) {
         while (count_ & 7 && data < end) {
             current_bytes[CURRENT_BYTES_IDX(count_ & 7)] = *data;
             ++data;
             ++count_;
         }

         /// If we still do not have enough bytes to an 8-byte word.
         if (count_ & 7) {
             return;
         }             

         v3_ ^= current_word;
         SIPROUND;
         SIPROUND;
         v0_ ^= current_word;
     }

     count_ += end - data;

     while (data + 8 <= end) {
         current_word = FromUnaligned<uint64_t>(data);

         v3_ ^= current_word;
         SIPROUND;
         SIPROUND;
         v0_ ^= current_word;

         data += 8;
     }

     /// Pad the remainder, which is missing up to an 8-byte word.
     current_word = 0;
     switch (end - data) {
     case 7: current_bytes[CURRENT_BYTES_IDX(6)] = data[6]; [[fallthrough]];
     case 6: current_bytes[CURRENT_BYTES_IDX(5)] = data[5]; [[fallthrough]];
     case 5: current_bytes[CURRENT_BYTES_IDX(4)] = data[4]; [[fallthrough]];
     case 4: current_bytes[CURRENT_BYTES_IDX(3)] = data[3]; [[fallthrough]];
     case 3: current_bytes[CURRENT_BYTES_IDX(2)] = data[2]; [[fallthrough]];
     case 2: current_bytes[CURRENT_BYTES_IDX(1)] = data[1]; [[fallthrough]];
     case 1: current_bytes[CURRENT_BYTES_IDX(0)] = data[0]; [[fallthrough]];
     case 0: break;
     }
 }

void SipHash::Update(const std::string& x) {
    Update(x.c_str(), x.length());
}

uint64_t SipHash::GetHash() const {
    Finalize();
    return v0_ ^ v1_ ^ v2_ ^ v3_;
}

void SipHash::Finalize() const {
    current_bytes[CURRENT_BYTES_IDX(7)] = static_cast<uint8_t>(count_);

    v3_ ^= current_word;
    SIPROUND;
    SIPROUND;
    v0_ ^= current_word;

    v2_ ^= 0xff;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    SIPROUND;
}

}
