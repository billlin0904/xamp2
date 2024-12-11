#include <base/rng.h>
#include <base/algorithm.h>
#include <base/platform.h>

XAMP_BASE_NAMESPACE_BEGIN

Sfc64Engine<> MakeRandomEngine() {
    using Clock = std::chrono::high_resolution_clock;
    uint32_t random_address = 0;
    // 利用系統級熵源 (GetSystemEntropy) 提供高品質初始亂數，再加上
    // 當前高精度時間戳 (Clock::now()) 的 count 產生 seed1。
    // 這使得種子值受系統底層亂數及時間因素影響，提高不可預測性。
    uint64_t seed1 = GetSystemEntropy()
        ^ static_cast<uint64_t>(Clock::now().time_since_epoch().count());

    // 使用 "黄金比例" 常數 0x9e3779b97f4a7c15ULL 與 seed1 相加，產生 seed2。
    // 該常數能將 bits 均勻擴散，減少模式出現的概率，進而提升隨機品質。
    uint64_t seed2 = seed1 + 0x9e3779b97f4a7c15ULL;

    // 將本地變數 random_address 的地址經由 std::hash 處理，以作為額外熵源，
    // seed3 將 seed2 與該 hash value XOR 結合。此舉利用位址隨機化 (ASLR)
    // 可能產生的變化，達到更高的不可預測性。
    uint64_t seed3 = seed2 ^ (std::hash<void*>()(&random_address));

    // 以 seed1, seed2, seed3 初始化 Sfc64Engine，引擎將在建構時
    // 執行預熱 (warmup) 12 回合，以確保內部狀態更均勻隨機化。
    // 使用 return 值直接建構物件，能讓編譯器做 RVO (Return Value Optimization) 
    // 減少不必要的拷貝或移動。
    return { seed1, seed2, seed3, 12 };
}

PRNG::PRNG() noexcept
    : engine_(MakeRandomEngine()) {
}

void PRNG::SetSeed(uint64_t seed) {
    engine_.seed(GetTime_t<std::chrono::milliseconds>() + seed);
}

std::string PRNG::GetRandomString(size_t size) {
    static constexpr std::string_view alphanum =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    std::string temp;
    temp.reserve(size);

    for (size_t i = 0; i < size; ++i) {
        temp += alphanum[NextInt32(0, alphanum.length() - 1)];
    }
    return temp;
}

XAMP_BASE_NAMESPACE_END