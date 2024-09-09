//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QFuture>

#include <string>
#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>

enum InvokeType {
	INVOKE_NONE,
	INVOKE_IMMEDIATELY,
};

class XAMP_WIDGET_SHARED_EXPORT BaseService : public QObject {
	Q_OBJECT
public:
	explicit BaseService(QObject* parent)
		: QObject(parent) {
	}

protected:
	template <typename Func>
	QFuture<std::invoke_result_t<Func>> invokeAsync(Func&& fun, InvokeType invoke_type = InvokeType::INVOKE_NONE) {
		using ReturnType = std::invoke_result_t<Func>;
		auto interface = std::make_shared<QFutureInterface<ReturnType>>();
		QMetaObject::invokeMethod(this, [interface, invoke_type, f = std::forward<Func>(fun), this]() {
			ReturnType val{};
			auto is_stop = is_stop_.load();
			if (invoke_type == InvokeType::INVOKE_IMMEDIATELY) {
				is_stop = false;
			}
			if (!is_stop) {
				try {
					val = f();
				}
				catch (const std::exception& e) {
					XAMP_LOG_E(logger_, "{} =>\r\n {}", e.what(), StackTrace{}.CaptureStack());
				}
			}
			interface->reportResult(val);
			interface->reportFinished();
			});
		return interface->future();
	}

	std::atomic<bool> is_stop_{ false };
	LoggerPtr logger_;
};

class XAMP_WIDGET_SHARED_EXPORT PythonInterop {
public:
	PythonInterop();

	XAMP_PIMPL(PythonInterop)
private:
	class PythonInteropImpl;
	AlignPtr<PythonInteropImpl> impl_;
};
