//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <atomic>
#include <cassert>

#include <base/base.h>
#include <output_device/output_device.h>

#include <unknwn.h>

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
		return refcount_.fetch_add(1, std::memory_order_relaxed) + 1;
	}

	/*
	* Release reference count
	* 
	* @return reference count
	*/
	ULONG STDMETHODCALLTYPE Release() override {
		const auto result = refcount_.fetch_sub(1, std::memory_order_acq_rel) - 1;
		if (result == 0) {
			delete this;
		}
		return result;
	}

protected:
	/*
	* Constructor
	*/
	UnknownImpl() : refcount_(0) {
	}

private:
	// Reference count
	std::atomic<ULONG> refcount_;
};

XAMP_OUTPUT_DEVICE_NAMESPACE_END

#endif // XAMP_OS_WIN

