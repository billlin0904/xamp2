//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <algorithm>
#include <stop_token>
#include <future>
#include <functional>

#include <base/base.h>
#include <base/memory.h>
#include <base/assert.h>

XAMP_BASE_NAMESPACE_BEGIN

/*
* MoveOnlyFunction is a function wrapper that can only be moved.
* It is used to wrap a function that is only called once.
*/
class XAMP_BASE_API MoveOnlyFunctionVTB final {
public:
    MoveOnlyFunctionVTB() noexcept = default;

    template <typename Func>
    MoveOnlyFunctionVTB(Func&& f)
        : impl_(MakeAlign<ImplBase, ImplType<Func>>(std::forward<Func>(f))) {
    }

	/*
	 * Calls the wrapped function with the given stop_token.
	 * The function is only called once.
	 */
    XAMP_ALWAYS_INLINE void operator()(const std::stop_token& stop_token) {
        XAMP_EXPECTS(impl_ != nullptr);
	    impl_->Invoke(stop_token);
        impl_.reset();
    }

    MoveOnlyFunctionVTB(MoveOnlyFunctionVTB&& other) noexcept
		: impl_(std::move(other.impl_)) {	    
    }

    MoveOnlyFunctionVTB& operator=(MoveOnlyFunctionVTB&& other) noexcept {
        impl_ = std::move(other.impl_);
        return *this;
    }

    explicit operator bool() const noexcept {
        return impl_ != nullptr;
    }
	
    XAMP_DISABLE_COPY(MoveOnlyFunctionVTB)
	
private:
    /*
    * ImplBase is a virtual base class for ImplType.
    * It is used to avoid the need to know the type of the function
    * when calling Invoke().
    */
    struct XAMP_NO_VTABLE ImplBase {
        virtual ~ImplBase() = default;
        virtual void Invoke(const std::stop_token& stop_token) = 0;
    };

    ScopedPtr<ImplBase> impl_;

    template <typename Func>
    struct ImplType final : ImplBase {
        static_assert(std::is_invocable_v<Func, const std::stop_token&>,
            "Func must be callable with a const StopToken& argument.");

	    ImplType(Func&& f) noexcept(std::is_nothrow_move_assignable_v<Func>)
            : f_(std::forward<Func>(f)) {
        }

        void Invoke(const std::stop_token& stop_token) override {
            std::invoke(f_, stop_token);
        }
        Func f_;
    };
};

/*
* MoveOnlyFunctionSFO 是一個只能被移動的函數包裝器。
* 它用於包裝只會被呼叫一次的函數。
* 優化點：
* 1. 小型函數優化 (SFO)
* 2. 移動語義 Invoke
* 3. 避免虛擬函數派發
* 4. 優化記憶體對齊
* 5. constexpr 支援
*/
class XAMP_BASE_API MoveOnlyFunctionSFO final {
private:
    // 小型函數優化的儲存大小，可以根據實際需求調整
    static constexpr size_t InlineStorageSize = 64;
    static constexpr size_t InlineAlignment = alignof(std::max_align_t);

    // 使用類型擦除而非虛擬函數的基類介面
    using InvokeFunc = void(*)(void*, const std::stop_token&);
    using DestroyFunc = void(*)(void*);
    using MoveFunc = void(*)(void*, void*);

    struct FunctionData {
        InvokeFunc invoke = nullptr;
        DestroyFunc destroy = nullptr;
        MoveFunc move = nullptr;
        bool uses_inline_storage = false;
    };

    // 內聯儲存空間
    alignas(InlineAlignment) char inline_storage_[InlineStorageSize];
    void* ptr_ = nullptr;
    FunctionData data_{};

    constexpr void clear() noexcept {
        if (ptr_) {
            data_.destroy(ptr_);
            if (!data_.uses_inline_storage) {
                ::operator delete(ptr_);
            }
            ptr_ = nullptr;
            data_ = FunctionData{};
        }
    }

    template<typename T>
    static constexpr bool CanUseInlineStorage() {
        return sizeof(T) <= InlineStorageSize &&
            alignof(T) <= InlineAlignment &&
            std::is_nothrow_move_constructible_v<T>;
    }

    template<typename T>
    static void invoke_impl(void* ptr, const std::stop_token& token) {
        T& func = *static_cast<T*>(ptr);
        std::invoke(func, token);
    }

    template<typename T>
    static void destroy_impl(void* ptr) {
        T* func = static_cast<T*>(ptr);
        func->~T();
    }

    template<typename T>
    static void move_impl(void* src, void* dst) {
        T* src_func = static_cast<T*>(src);
        new (dst) T(std::move(*src_func));
        src_func->~T();
    }

public:
    // 默認構造函數
    constexpr MoveOnlyFunctionSFO() noexcept = default;

    // 從可調用物件構造
    template <typename Func,
        typename = std::enable_if_t<std::is_invocable_v<Func, const std::stop_token&>>>
    MoveOnlyFunctionSFO(Func&& f) {
        using DecayFunc = std::decay_t<Func>;
        static_assert(!std::is_same_v<DecayFunc, MoveOnlyFunctionSFO>,
            "Cannot create MoveOnlyFunction from another MoveOnlyFunction");

        data_.invoke = &invoke_impl<DecayFunc>;
        data_.destroy = &destroy_impl<DecayFunc>;
        data_.move = &move_impl<DecayFunc>;

        if constexpr (CanUseInlineStorage<DecayFunc>()) {
            // 使用內聯儲存
            ptr_ = static_cast<void*>(inline_storage_);
            data_.uses_inline_storage = true;
            new (ptr_) DecayFunc(std::forward<Func>(f));
        }
        else {
            // 使用動態分配
            ptr_ = ::operator new(sizeof(DecayFunc), std::align_val_t{ alignof(DecayFunc) });
            data_.uses_inline_storage = false;
            new (ptr_) DecayFunc(std::forward<Func>(f));
        }
    }

    // 移動構造函數
    constexpr MoveOnlyFunctionSFO(MoveOnlyFunctionSFO&& other) noexcept {
        if (other.ptr_) {
            data_ = other.data_;

            if (other.data_.uses_inline_storage) {
                // 從其他物件的內聯儲存移動
                ptr_ = static_cast<void*>(inline_storage_);
                data_.move(other.ptr_, ptr_);
            }
            else {
                // 接管動態分配的記憶體
                ptr_ = other.ptr_;
                other.ptr_ = nullptr;
                other.data_ = FunctionData{};
            }
        }
    }

    // 移動賦值運算符
    constexpr MoveOnlyFunctionSFO& operator=(MoveOnlyFunctionSFO&& other) noexcept {
        if (this != &other) {
            clear();

            if (other.ptr_) {
                data_ = other.data_;

                if (other.data_.uses_inline_storage) {
                    // 從其他物件的內聯儲存移動
                    ptr_ = static_cast<void*>(inline_storage_);
                    data_.move(other.ptr_, ptr_);
                }
                else {
                    // 接管動態分配的記憶體
                    ptr_ = other.ptr_;
                    other.ptr_ = nullptr;
                    other.data_ = FunctionData{};
                }
            }
        }
        return *this;
    }

    // 析構函數
    ~MoveOnlyFunctionSFO() {
        clear();
    }

    // 呼叫運算符 - 只能被呼叫一次
    XAMP_ALWAYS_INLINE void operator()(const std::stop_token& stop_token) {
        XAMP_EXPECTS(ptr_ != nullptr);
        // 保存函數指針，以便在調用後清除
        auto invoke_fn = data_.invoke;
        void* fn_ptr = ptr_;

        // 清除函數狀態，確保它只能被調用一次
        auto destroy_fn = data_.destroy;
        bool was_inline = data_.uses_inline_storage;

        ptr_ = nullptr;
        data_ = FunctionData{};

        // 調用函數
        invoke_fn(fn_ptr, stop_token);

        // 清理資源
        destroy_fn(fn_ptr);
        if (!was_inline) {
            ::operator delete(fn_ptr);
        }
    }

    // 顯式布爾轉換
    constexpr explicit operator bool() const noexcept {
        return ptr_ != nullptr;
    }

    // 檢查函數是否有效
    constexpr bool valid() const noexcept {
        return ptr_ != nullptr;
    }

    XAMP_DISABLE_COPY(MoveOnlyFunctionSFO)
};

using MoveOnlyFunction = MoveOnlyFunctionSFO;

XAMP_BASE_NAMESPACE_END

