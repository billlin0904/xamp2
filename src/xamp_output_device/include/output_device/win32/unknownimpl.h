//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <atomic>
#include <cassert>

#include <base/base.h>
#include <output_device/win32/wasapi.h>

#ifdef XAMP_OS_WIN

XAMP_OUTPUT_DEVICE_NAMESPACE_BEGIN

/*
* UnknownImpl is the class for IUnknown.
* 
* @tparam Base class type.
*/
template <typename Base>
class UnknownImpl : public Base {
public:
	XAMP_DISABLE_COPY(UnknownImpl)

	/*
	* Destructor
	*/
	virtual ~UnknownImpl() = default;	

	/*
	* Add reference count
	* 
	* @return reference count
	*/	
	ULONG STDMETHODCALLTYPE AddRef() override {
		auto result = refcount_.fetch_add(1);
		return result;
	}

	/*
	* Release reference count
	* 
	* @return reference count
	*/
	ULONG STDMETHODCALLTYPE Release() override {
		auto result = refcount_.fetch_sub(1);
		return result;
	}

protected:
	/*
	* Constructor
	*/
	UnknownImpl() noexcept
		: refcount_(0) {
	}

private:
	// Reference count
	std::atomic<ULONG> refcount_;
};

XAMP_OUTPUT_DEVICE_NAMESPACE_END

#endif // XAMP_OS_WIN

