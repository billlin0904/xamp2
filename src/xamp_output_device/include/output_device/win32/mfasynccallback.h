//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <output_device/win32/unknownimpl.h>
#include <base/assert.h>

#include <windows.h>
#include <mfidl.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

/*
* MFAsyncCallback is the class for IMFAsyncCallback.
* 
* @tparam ParentClass Parent class type.
*/
template <typename ParentClass>
class MFAsyncCallback final : public UnknownImpl<IMFAsyncCallback> {
public:
	/*
	* Callback function type
	* 
	* @param[in] result: IMFAsyncResult pointer.
	* @return HRESULT
	*/
	typedef HRESULT(ParentClass::*Callback)(IMFAsyncResult *);

	/*
	* Constructor.
	* 
	* @param[in] parent: Parent class.
	* @param[in] fn: Callback function.
	* @param[in] queue_id: Queue id.
	*/
	MFAsyncCallback(ParentClass* parent, const Callback fn, const DWORD queue_id)
		: queue_id_(queue_id)
		, parent_(parent)
		, callback_(fn) {
		XAMP_ASSERT(parent != nullptr);
		XAMP_ASSERT(fn != nullptr);
	}

	/*
	* Destructor.
	*/
	virtual ~MFAsyncCallback() = default;

	/*
	* Query interface.
	* 
	* @param[in] iid: Interface id.
	* @param[out] ppv: Interface pointer.	
	*/
	STDMETHODIMP QueryInterface(REFIID iid, void** ppv) override {
		if (!ppv) {
			return E_POINTER;
		}

		if (iid == __uuidof(IUnknown)) {
			*ppv = static_cast<IUnknown*>(static_cast<IMFAsyncCallback*>(this));
		} else if (iid == __uuidof(IMFAsyncCallback)) {
			*ppv = static_cast<IMFAsyncCallback*>(this);
		} else {
			*ppv = nullptr;
			return E_NOINTERFACE;
		}

		AddRef();
		return S_OK;
	}

	/*
	* Get parameters.
	* 
	* @param[out] flags: Flags.
	* @param[out] queue: Queue id.
	*/
	STDMETHODIMP GetParameters(DWORD* flags, DWORD* queue) override {
		*flags = 0;
		*queue = queue_id_;
		return S_OK;
	}

	/*
	* Invoke.
	* 
	* @param[in] async_result: Async result.	
	*/
	STDMETHODIMP Invoke(IMFAsyncResult* async_result) override {
		return (parent_->*callback_)(async_result);
	}

private:
	// Queue id
	DWORD queue_id_;	
	// Parent class
	ParentClass *parent_;
	// Callback function
	const Callback callback_;
};

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif // XAMP_OS_WIN
