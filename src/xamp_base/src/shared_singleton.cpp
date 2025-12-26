#include <base/shared_singleton.h>
#include <base/platform.h>
#include <base/fastmutex.h>
#include <base/logger_impl.h>
#include <base/stl.h>

#include <mutex>

XAMP_BASE_NAMESPACE_BEGIN

namespace {
	struct StringHash {
		using is_transparent = void;

		size_t operator()(std::string_view v) const noexcept {
			return std::hash<std::string_view>{}(v);
		}
		size_t operator()(const std::string& s) const noexcept {
			return std::hash<std::string_view>{}(s);
		}
	};

	struct StringEqual {
		using is_transparent = void;

		bool operator()(std::string_view lhs, std::string_view rhs) const noexcept {
			return lhs == rhs;
		}
		bool operator()(const std::string& lhs, const std::string& rhs) const noexcept {
			return lhs == rhs;
		}
	};

	/*
	* ObjectInstance is a struct that contains the singleton instance and the mutex of the singleton.
	*
	*/
	struct ObjectInstance {
		void* object{ nullptr };
		std::shared_ptr<FastMutex> mutex{ std::make_shared<FastMutex>() };
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

	using SlotPtr = std::shared_ptr<ObjectInstance>;
	HashMap<std::string, SlotPtr, StringHash, StringEqual> object_type_lut;

	SlotPtr GetSingletonByType(std::string_view name) {
		auto itr = object_type_lut.find(name);
		if (itr != object_type_lut.end()) {
			return itr->second;
		}		
		auto slot = std::make_shared<ObjectInstance>();
		object_type_lut.emplace(name, slot);
		return slot;
	}
}

void GetSharedInstance(std::string_view type_name,
	void* (*get_static_instance)(),
	void*& instance) {
	SlotPtr ptr;

	{
		std::lock_guard<FastMutex> guard{ GetSingletonMutex() };
		if (instance != nullptr) {
			return;
		}
		ptr = GetSingletonByType(type_name);
	}

	const bool is_logger = (type_name == LoggerManager::GetSingletonName());

	{
		std::lock_guard<FastMutex> guard(*ptr->mutex);
		if (ptr->object == nullptr) {
			if (!is_logger) {
				XAMP_LOG_INFO("Creating shared singleton instance for type '{}'.", type_name);
			}
			ptr->object = (*get_static_instance)();
		}
	}

	{
		std::lock_guard<FastMutex> guard{ GetSingletonMutex() };
		instance = ptr->object;
	}
}

XAMP_BASE_NAMESPACE_END