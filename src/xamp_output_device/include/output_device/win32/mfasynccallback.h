//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <cassert>
#include <Windows.h>
#include <Mfidl.h>

namespace xamp::output_device::win32 {

template <typename ParentType>
class MFAsyncCallback final : public IMFAsyncCallback {
public:
	typedef HRESULT(ParentType::*Callback)(IMFAsyncResult *);

	MFAsyncCallback(ParentType* parent, const Callback fn, const DWORD queue_id)
		: refcount_(0)
		, queue_id_(queue_id)
		, parent_(parent)
		, callback_(fn) {
		assert(parent != nullptr);
		assert(fn != nullptr);
	}

	STDMETHODIMP_(ULONG) AddRef() override {
		return ::InterlockedIncrement(&refcount_);
	}

	STDMETHODIMP_(ULONG) Release() override {
		const ULONG value = ::InterlockedDecrement(&refcount_);
		if (value == 0) {
			delete this;
		}
		return value;
	}

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

	DWORD GetWorkQueueId() const noexcept {
		return queue_id_;
	}

	STDMETHODIMP GetParameters(DWORD* flags, DWORD* queue) override {
		*flags = 0;
		*queue = queue_id_;
		return S_OK;
	}

	STDMETHODIMP Invoke(IMFAsyncResult* async_result) override {
		return (parent_->*callback_)(async_result);
	}

private:
	LONG refcount_;
	DWORD queue_id_;
	ParentType *parent_;
	const Callback callback_;
};

}
#endif
