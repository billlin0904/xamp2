//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#include "base/fastmutex.h"

#ifdef XAMP_OS_WIN

#include <output_device/win32/comexception.h>
#include <output_device/win32/unknownimpl.h>

#include <base/assert.h>

#include <atlbase.h>
#include <mfapi.h>
#include <shlwapi.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

template <typename ParentClass>
class WasapiWorkQueue : public UnknownImpl<IMFAsyncCallback> {
public:
	typedef HRESULT(ParentClass::* Callback)(IMFAsyncResult*);

	WasapiWorkQueue(const std::wstring &mmcss_name, ParentClass* parent, const Callback fn)
		: mmcss_name_(mmcss_name)
		, queue_id_(MAXDWORD)
		, shared_queue_id_(MAXDWORD)
		, task_id_(0)
		, workitem_key_(0)
		, parent_(parent)
		, callback_(fn) {
		XAMP_EXPECTS(!mmcss_name.empty());
		XAMP_EXPECTS(parent != nullptr);
		XAMP_ASSERT(fn != nullptr);
	}

	virtual ~WasapiWorkQueue() noexcept override {
		destroy();
	}

	bool isValid() const {
		return queue_id_ != MAXDWORD;
	}

	void initial() {
		DWORD shared_queue_id = MF_MULTITHREADED_WORKQUEUE;
		hrIfFailThrow(::MFLockSharedWorkQueue(mmcss_name_.c_str(), 0, &task_id_, &shared_queue_id));

		DWORD queue_id = MAXDWORD;
		CComPtr<IMFAsyncResult> async_result;
		try {
			hrIfFailThrow(::MFAllocateSerialWorkQueue(shared_queue_id, &queue_id));
			hrIfFailThrow(::MFCreateAsyncResult(nullptr, this, nullptr, &async_result));
		}
		catch (...) {
			if (queue_id != MAXDWORD) {
				::MFUnlockWorkQueue(queue_id);
			}
			::MFUnlockWorkQueue(shared_queue_id);
			task_id_ = 0;
			throw;
		}

		shared_queue_id_ = shared_queue_id;
		queue_id_ = queue_id;
		async_result_ = async_result;
	}

	void waitAsync(HANDLE event) {
		std::lock_guard<SpinLock> guard{ mutex_ };
		if (!isValid()) {
			return;
		}
		workitem_key_ = 0;
		hrIfFailThrow(::MFPutWaitingWorkItem(event, 1, async_result_, &workitem_key_));
	}

	void destroy() noexcept {
		std::lock_guard<SpinLock> guard{ mutex_ };
		if (workitem_key_ != 0) {
			(void)::MFCancelWorkItem(workitem_key_);
			workitem_key_ = 0;
		}

		async_result_.Release();

		if (queue_id_ != MAXDWORD) {
			(void)::MFUnlockWorkQueue(queue_id_);
			queue_id_ = MAXDWORD;
		}
		if (shared_queue_id_ != MAXDWORD) {
			(void)::MFUnlockWorkQueue(shared_queue_id_);
			shared_queue_id_ = MAXDWORD;
		}
		task_id_ = 0;
	}

	STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
		static const QITAB qit[] = {
			QITABENT(WasapiWorkQueue, IUnknown),
			QITABENT(WasapiWorkQueue, IMFAsyncCallback),
			{ nullptr },
		};
		return QISearch(this, qit, riid, ppv);
	}

	STDMETHODIMP GetParameters(DWORD* flags, DWORD* queue) override {
		*flags = 0;
		*queue = queue_id_;
		return S_OK;
	}	

	STDMETHODIMP Invoke(IMFAsyncResult* async_result) override {
		{
			std::lock_guard<SpinLock> guard{ mutex_ };
			if (!isValid()) {
				return S_OK;
			}
		}
		return (parent_->*callback_)(async_result);
	}	

private:
	SpinLock mutex_;
	std::wstring mmcss_name_;
	DWORD queue_id_;
	DWORD shared_queue_id_;
	DWORD task_id_;
	MFWORKITEM_KEY workitem_key_;
	ParentClass* parent_;
	CComPtr<IMFAsyncResult> async_result_;
	const Callback callback_; 
};

template <typename T>
CComPtr<WasapiWorkQueue<T>> makeWasapiWorkQueue(const std::wstring& mmcss_name,
	T* ptr,
	typename WasapiWorkQueue<T>::Callback callback) {
	return CComPtr<WasapiWorkQueue<T>>(new WasapiWorkQueue<T>(mmcss_name,
		ptr,
		callback));
}

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif
