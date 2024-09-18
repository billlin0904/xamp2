#include <base/fastconditionvariable.h>

XAMP_BASE_NAMESPACE_BEGIN

void FastConditionVariable::wait(std::unique_lock<FastMutex>& lock) {
    auto* mutex = lock.mutex();
    while (!condition_met_.load(std::memory_order_acquire)) {
        // ���o�ª����A�A�ϥ� memory_order_acquire �O�Ҥ��s�i����
        auto old_state = state_.load(std::memory_order_acquire);
        // ���꤬����
        mutex->unlock();
        // ���ݱ����ܼ�
        AtomicWait(state_, old_state, kInfinity);
        // �A����w������
        mutex->lock();
        // �A���ˬd����
    }
}

void FastConditionVariable::notify_one() noexcept {
    // �ק�@�ɪ��A�A�ϥ� memory_order_release
    condition_met_.store(true, std::memory_order_release);
    // �W�[��w��A�ϥ� memory_order_release
    state_.fetch_add(kLocked, std::memory_order_release);
    // ����@�ӽu�{
    AtomicWakeSingle(state_);
}

void FastConditionVariable::notify_all() noexcept {
    // �ק�@�ɪ��A�A�ϥ� memory_order_release
    condition_met_.store(true, std::memory_order_release);
    // �W�[��w��A�ϥ� memory_order_release
    state_.fetch_add(kLocked, std::memory_order_release);
    // ����Ҧ��u�{
    AtomicWakeAll(state_);
}

XAMP_BASE_NAMESPACE_END
