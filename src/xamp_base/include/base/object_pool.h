//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

namespace xamp::base {

template <typename T>
class DefaultFactory {
public:
    T *Create() {
        return new T();
    }
};

template 
<
    typename T, typename FactoryType = DefaultFactory<T>
>
class ObjectPool : public std::enable_shared_from_this<ObjectPool<T, FactoryType>> {
private:
    class ReturnToPool;

    using factory_type = FactoryType;
    using pool_type = ObjectPool<T, FactoryType>;
    using deleter_type = pool_type::return_to_pool;
    using ptr_type = std::unique_ptr<T>;

public:
    using return_ptr_type = std::unique_ptr<T, deleter_type>;

    explicit ObjectPool(size_t _init_size)
        : max_size(2 * init_size)
        , current_size(0)
        , init_size(_init_size) {
        init();
    }

    ObjectPool(size_t _init_size, typename remove_reference<factory_type>::type &&factory)
        : max_size(2 * _init_size)
        , current_size(0)
        , init_size(_init_size)
        , factory(std::move(factory)) {
        init(); 
    }

    ObjectPool(size_t _init_size,
               typename std::conditional<std::is_reference<factory_type>::value, factory_type, const factory_type &>::type factory)
        : max_size(2 * _init_size)
        , current_size(0)
        , init_size(_init_size) {
        this->factory = factory;
        init();
    }
    
    void Release(ptr_type &object) {
        std::unique_lock<mutex> lck(this->object_mutex);
        this->objects.push_back(move(object));
        idle_cv.notify_one();
    }

    return_ptr_type Acquire() {
        std::unique_lock<mutex> lck(this->object_mutex);

        while (true) {
            auto size = this->objects.size();

            if (size > 0) {
                return_ptr_type ptr(this->objects.back().release(), deleter_type{this->shared_from_this()});
                this->objects.pop_back();
                return ptr;
            } else {
                auto obj = this->create_object();
                if (obj != nullptr)
                    return return_ptr_type(obj, deleter_type{this->shared_from_this()});
                idle_cv.wait(lck);
            }
        }
    }

private:
    size_t current_size;
    size_t max_size;
    size_t init_size;

    std::mutex object_mutex;
    std::condition_variable idle_cv;

    factory_type factory;
    std::vector<std::unique_ptr<T>> objects;

    inline T *CreateObject() {
        if (current_size < max_size) {
            this->current_size++;
            return factory.Create();
        }
        else {
            return nullptr;
        }
    }

    inline void Init()  {
        for (int i = 0; i < init_size; ++i) {
            objects.emplace_back(this->CreateObject());
        }
    }

    class ReturnToPool {
    private:
        weak_ptr<pool_type> pool;

    public:
        explicit ReturnToPool(const std::shared_ptr<pool_type> &ptr)
            : pool(ptr) {
        }

        void operator()(type *object) {
            ptr_type ptr(object);
            if (auto sp = this->pool.lock()) {
                try {
                    sp->Release(ptr);
                } catch (...) {
                }
            }
        }
    };
};

}

