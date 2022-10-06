#include <mutex>
#include <base/fastmutex.h>
#include <base/stl.h>
#include <base/shared_singleton.h>

namespace xamp::base {

struct ObjectInstance {
	void* object {nullptr};
	std::shared_ptr<FastMutex> mutex;
};

static FastMutex& GetSingletonMutex() {
	static FastMutex mutex;
	return mutex;
}

static ObjectInstance* GetSingletonByType(const std::type_index& type_index) {
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

}
