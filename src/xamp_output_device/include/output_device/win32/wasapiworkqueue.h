//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <output_device/win32/hrexception.h>
#include <output_device/win32/unknownimpl.h>

#include <base/assert.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

/*
* WasapiWorkQueue is wasapi work queue.
* 
* @tparam ParentClass: parent class.
*/
template <typename ParentClass>
class WasapiWorkQueue : public UnknownImpl<IMFAsyncCallback> {
public:
	/*
	* Callback function.
	* 
	* @param[in] result: async result.
	*/
	typedef HRESULT(ParentClass::* Callback)(IMFAsyncResult*);

	/*
	* Constructor.
	* 
	* @param[in] mmcss_name: mmcss name.
	* @param[in] parent: parent class.
	* @param[in] fn: callback function.
	*/
	WasapiWorkQueue(const std::wstring &mmcss_name, ParentClass* parent, const Callback fn)
		: mmcss_name_(mmcss_name)
		, queue_id_(MAXDWORD)
		, task_id_(0)
		, workitem_key_(0)
		, parent_(parent)
		, callback_(fn) {
		XAMP_EXPECTS(!mmcss_name.empty());
		XAMP_EXPECTS(parent != nullptr);
		XAMP_ASSERT(fn != nullptr);
	}

	/*
	* Destructor.
	*/
	virtual ~WasapiWorkQueue() override {
		Destroy();
	}

	/*
	* Get queue id.
	* 
	* @return bool
	*/
	bool IsValid() const noexcept {
		return queue_id_ != MAXDWORD;
	}

	/*
	* Destroy.
	* 
	*/
	void Destroy() {
		if (!IsValid()) {
			return;
		}
		async_result_.Release();
		HrIfFailledThrow(::MFUnlockWorkQueue(queue_id_));
		queue_id_ = MAXDWORD;
		task_id_ = 0;
	}

	/*
	* QueryInterface.
	* 
	* @param[in] iid: interface id.
	* @param[in] ppv: object.
	*/
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

	/*
	* GetParameters.
	* 
	* @param[in] flags: flags.
	* @param[in] queue: queue id.
	*/
	STDMETHODIMP GetParameters(DWORD* flags, DWORD* queue) override {
		*flags = 0;
		*queue = queue_id_;
		return S_OK;
	}

	/*
	* Initial.
	*/
	void Initial() {
		DWORD queue_id = MF_MULTITHREADED_WORKQUEUE;
		HrIfFailledThrow(::MFLockSharedWorkQueue(mmcss_name_.c_str(), 0, &task_id_, &queue_id));
		queue_id_ = queue_id;
		HrIfFailledThrow(::MFCreateAsyncResult(nullptr, this, nullptr, &async_result_));
	}

	/*
	* Invoke.
	* 
	* @param[in] async_result: async result.
	*/
	STDMETHODIMP Invoke(IMFAsyncResult* async_result) override {
		if (!IsValid()) {
			return S_OK;
		}
		return (parent_->*callback_)(async_result);
	}

	/*
	* Wait async.
	* 
	*/
	void WaitAsync(HANDLE event) {
		workitem_key_ = 0;
		HrIfFailledThrow(::MFPutWaitingWorkItem(event, 1, async_result_, &workitem_key_));
	}

private:
	std::wstring mmcss_name_;
	DWORD queue_id_;
	DWORD task_id_;
	MFWORKITEM_KEY workitem_key_;
	ParentClass* parent_;
	CComPtr<IMFAsyncResult> async_result_;
	const Callback callback_; 
};

template <typename T>
CComPtr<WasapiWorkQueue<T>> MakeWasapiWorkQueue(const std::wstring& mmcss_name,
	T* ptr, typename WasapiWorkQueue<T>::Callback callback) {
	return CComPtr<WasapiWorkQueue<T>>(new WasapiWorkQueue<T>(mmcss_name,
		ptr,
		callback));
}

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif // XAMP_OS_WIN
