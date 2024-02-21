#include <base/shared_singleton.h>

#include <base/fastmutex.h>
#include <base/stl.h>

#include <mutex>

XAMP_BASE_NAMESPACE_BEGIN

namespace {
	/*
	* ObjectInstance is a struct that contains the singleton instance and the mutex of the singleton.
	*
	*/
	struct ObjectInstance {
		void* object{ nullptr };
		std::shared_ptr<FastMutex> mutex;
	};

	/*
	* GetSingletonMutex is a function that returns the mutex of the singleton.
	*
	* @return The mutex of the singleton.
	*/
	FastMutex& GetSingletonMutex() {
		static FastMutex mutex;
		return mutex;
	}

	/*
	* GetSingletonByType is a function that returns the singleton instance by type.
	*
	* @param[in] type_index The type index of the singleton instance.
	* @return The singleton instance.
	*
	*/
	ObjectInstance* GetSingletonByType(const std::type_index& type_index) {
		static HashMap<std::type_index, ObjectInstance> object_type_lut;

		auto itr = object_type_lut.find(type_index);
		if (itr != object_type_lut.end()) {
			return &itr->second;
		}

		itr = object_type_lut.emplace(
			std::make_pair(type_index, ObjectInstance())).first;
		itr->second.object = nullptr;
		itr->second.mutex = std::make_shared<FastMutex>();

		return &itr->second;
	}
}

void GetSharedInstance(const std::type_index& type_index,
	void* (*get_static_instance)(),
	void*& instance) {
	ObjectInstance* ptr = nullptr;

	{
		std::lock_guard<FastMutex> guard{ GetSingletonMutex() };
		if (instance != nullptr) {
			return;
		}
		ptr = GetSingletonByType(type_index);
	}
	{
		std::lock_guard<FastMutex> guard(*ptr->mutex);
		if (ptr->object == nullptr) {
			ptr->object = (*get_static_instance)();
		}
	}
	{
		std::lock_guard<FastMutex> guard{ GetSingletonMutex() };
		instance = ptr->object;
	}
}

XAMP_BASE_NAMESPACE_END