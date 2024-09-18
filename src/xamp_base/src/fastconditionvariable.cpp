#include <base/fastconditionvariable.h>

XAMP_BASE_NAMESPACE_BEGIN

void FastConditionVariable::wait(std::unique_lock<FastMutex>& lock) {
    auto* mutex = lock.mutex();
    while (!condition_met_.load(std::memory_order_acquire)) {
        // 取得舊的狀態，使用 memory_order_acquire 保證內存可見性
        auto old_state = state_.load(std::memory_order_acquire);
        // 解鎖互斥鎖
        mutex->unlock();
        // 等待條件變數
        AtomicWait(state_, old_state, kInfinity);
        // 再次鎖定互斥鎖
        mutex->lock();
        // 再次檢查條件
    }
}

void FastConditionVariable::notify_one() noexcept {
    // 修改共享狀態，使用 memory_order_release
    condition_met_.store(true, std::memory_order_release);
    // 增加鎖定位，使用 memory_order_release
    state_.fetch_add(kLocked, std::memory_order_release);
    // 喚醒一個線程
    AtomicWakeSingle(state_);
}

void FastConditionVariable::notify_all() noexcept {
    // 修改共享狀態，使用 memory_order_release
    condition_met_.store(true, std::memory_order_release);
    // 增加鎖定位，使用 memory_order_release
    state_.fetch_add(kLocked, std::memory_order_release);
    // 喚醒所有線程
    AtomicWakeAll(state_);
}

XAMP_BASE_NAMESPACE_END
