#pragma once

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace xamp::base {
#if defined(__cpp_lib_move_only_function) && __cpp_lib_move_only_function >= 202110L
template<class Signature>
using MoveOnlyFunction = std::move_only_function<Signature>;
#else
// libc++ versions without C++23 move_only_function still need move-only tasks.
template<class Signature> class MoveOnlyFunction;
template<class R, class... Args>
class MoveOnlyFunction<R(Args...)> {
    struct Callable {
        virtual ~Callable() = default;
        virtual R call(Args... args) = 0;
    };
    template<class F> struct Model final : Callable {
        F function;
        template<class G> explicit Model(G&& f) : function(std::forward<G>(f)) {}
        R call(Args... args) override { return std::invoke(function, std::forward<Args>(args)...); }
    };
    std::unique_ptr<Callable> callable_;
public:
    MoveOnlyFunction() = default;
    MoveOnlyFunction(std::nullptr_t) noexcept {}
    MoveOnlyFunction(MoveOnlyFunction&&) noexcept = default;
    MoveOnlyFunction& operator=(MoveOnlyFunction&&) noexcept = default;
    MoveOnlyFunction(const MoveOnlyFunction&) = delete;
    MoveOnlyFunction& operator=(const MoveOnlyFunction&) = delete;
    template<class F> requires (!std::is_same_v<std::remove_cvref_t<F>, MoveOnlyFunction>
        && std::is_invocable_r_v<R, std::decay_t<F>&, Args...>)
    MoveOnlyFunction(F&& f) : callable_(std::make_unique<Model<std::decay_t<F>>>(std::forward<F>(f))) {}
    MoveOnlyFunction& operator=(std::nullptr_t) noexcept { callable_.reset(); return *this; }
    friend bool operator==(const MoveOnlyFunction& f, std::nullptr_t) noexcept { return !f.callable_; }
    explicit operator bool() const noexcept { return bool(callable_); }
    R operator()(Args... args) { return callable_->call(std::forward<Args>(args)...); }
};
#endif
}
