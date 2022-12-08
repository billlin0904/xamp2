//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <typeindex>

#include <base/base.h>
#include <base/stl.h>
#include <base/align_ptr.h>
#include <base/exception.h>

namespace xamp::base {

// https://gist.github.com/Jiwan/aefcb797c31911377b1c1c679ca5f1cd

struct XAMP_BASE_API IContainer {
	virtual ~IContainer() = default;
};

template <typename T>
struct Container : IContainer {
	Container(AlignPtr<T> p)
		: instanace(std::move(p)) {
	}
	AlignPtr<T> instanace;
};

struct ServiceProvider;

template <typename T>
struct GenericArgument {
	template
	<
		typename Type, 
		std::enable_if_t<!std::is_same_v<T, std::decay_t<Type>>, int> = 0
	> 
	constexpr operator Type&() const noexcept {
		return provider.GetRequiredService<Type>();
	}

	ServiceProvider& provider;
};

class XAMP_BASE_API ServiceProvider {
public:
	ServiceProvider() = default;

	XAMP_DISABLE_COPY(ServiceProvider)

	template <typename T>
	T& GetRequiredService() {
		const auto type_index = std::type_index{ typeid(T) };
		auto it = instances_.find(type_index);
		if (it != instances_.end()) {
			return *static_cast<Container<T>*>(it->second.get())->instanace;
		}
		return FindOrCreateInstance<T>();
	}

	template
	<
		typename Interface, typename Implements
	>
	void AddService() {
		creator_.emplace(std::type_index{ typeid(Interface) }, [this]() {
			instances_.emplace(std::type_index{ typeid(Interface) }, CreateImpl<Implements>(*this));
			});
	}
private:
	template
	<
		typename T,
		std::enable_if_t<!std::is_abstract_v<T>, int> = 0
	>
	T& FindOrCreateInstance() {
		const auto type_index = std::type_index{ typeid(T) };
		auto creator_it = creator_.find(type_index);
		if (creator_it != creator_.end()) {
			creator_it->second();
			return *static_cast<Container<T>*>(instances_[type_index].get())->component;
		}

		auto result = instances_.emplace(type_index, CreateImpl<T>(*this));
		return *static_cast<Container<T>*>(result.first->second.get())->component;
	}

	template
	<
		typename T,
		std::enable_if_t<std::is_abstract_v<T>, int> = 0
	>
	T& FindOrCreateInstance() {
		const auto type_index = std::type_index{ typeid(T) };
		auto creator_it = creator_.find(type_index);
		if (creator_it != creator_.end()) {
			creator_it->second();
			return *static_cast<Container<T>*>(instances_[type_index].get())->component;
		}

		throw Exception("No implemented class provided.");
	}

	template <typename T, typename... Arguments>
	constexpr auto CreateImpl(ServiceProvider& di)
		-> std::enable_if_t<std::is_constructible_v<T, Arguments...>, AlignPtr<Container<T>>> {
		return MakeAlign<Container<T>>(MakeAlign<T>(Arguments{ di }...));
	}

	template <typename T, typename... Arguments>
	constexpr auto CreateImpl(ServiceProvider& di)
		-> std::enable_if_t<!std::is_constructible_v<T, Arguments...>, AlignPtr<Container<T>>> {
		return CreateImpl<T, Arguments..., GenericArgument<T>>(di);
	}

	HashMap<std::type_index, std::function<void()>> creator_;
	HashMap<std::type_index, AlignPtr<IContainer>> instances_;
};

}