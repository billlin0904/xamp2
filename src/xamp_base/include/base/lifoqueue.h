#pragma once

#include <stack>

namespace xamp::base {

template <typename T>
class LIFOQueue {
public:
    explicit LIFOQueue(size_t size) {
    }

    void emplace(T&& item) {
        queue_.emplace(std::move(item));
    }

    T& top() {
        return queue_.top();
    }

    void pop() {
        queue_.pop();
    }

    bool empty() const {
        return queue_.empty();
    }

    size_t size() const {
        return queue_.size();
    }
private:
    std::stack<T> queue_;
};

}

