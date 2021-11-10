//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <output_device/win32/hrexception.h>
#include <output_device/win32/unknownimpl.h>

namespace xamp::output_device::win32 {

template <typename ParentType>
class WASAPIWorkQueue final : public UnknownImpl<IMFAsyncCallback> {
public:
	typedef HRESULT(ParentType::* Callback)(IMFAsyncResult*);

	RtWorkQueue(const std::wstring &mmcss_name, ParentType* parent, const Callback fn)
		: queue_id_(MF_MULTITHREADED_WORKQUEUE)
		, task_id_(0)
		, workitem_key_(0)
		, parent_(parent)
		, callback_(fn) {
		assert(parent != nullptr);
		assert(fn != nullptr);
		HrIfFailledThrow(::MFLockSharedWorkQueue(mmcss_name.c_str(), 0, &task_id_, &queue_id_));
		HrIfFailledThrow(::MFCreateAsyncResult(nullptr, this, nullptr, &async_result_));
	}

	~RtWorkQueue() override {
		Stop();
	}

	void Stop() {
		async_result_.Release();
		HrIfFailledThrow(::MFUnlockWorkQueue(queue_id_));
	}

	STDMETHODIMP QueryInterface(REFIID iid, void** ppv) override {
		if (!ppv) {
			return E_POINTER;
		}

		if (iid == __uuidof(IUnknown)) {
			*ppv = static_cast<IUnknown*>(static_cast<IMFAsyncCallback*>(this));
		}
		else if (iid == __uuidof(IMFAsyncCallback)) {
			*ppv = static_cast<IMFAsyncCallback*>(this);
		}
		else {
			*ppv = nullptr;
			return E_NOINTERFACE;
		}

		AddRef();
		return S_OK;
	}

	STDMETHODIMP GetParameters(DWORD* flags, DWORD* queue) override {
		*flags = 0;
		*queue = queue_id_;
		return S_OK;
	}

	STDMETHODIMP Invoke(IMFAsyncResult* async_result) override {
		return (parent_->*callback_)(async_result);
	}

	void Queue(HANDLE event) {
		HrIfFailledThrow(::MFPutWaitingWorkItem(event, 1, async_result_, &workitem_key_));
	}

private:
	DWORD queue_id_;
	DWORD task_id_;
	MFWORKITEM_KEY workitem_key_;
	ParentType* parent_;
	CComPtr<IMFAsyncResult> async_result_;
	const Callback callback_; 
};

template <typename T>
CComPtr<WASAPIWorkQueue<T>> MakeRtWorkQueue(T* ptr, typename WASAPIWorkQueue<T>::Callback callback, DWORD queue_id) {
	return CComPtr<WASAPIWorkQueue<T>>(new WASAPIWorkQueue<T>(ptr,
		callback,
		queue_id));
}

}
