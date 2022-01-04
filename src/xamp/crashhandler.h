//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#if  0
#pragma once

#include <base/align_ptr.h>

#include <QString>

using xamp::base::AlignPtr;

class CrashHandler {
public:
	static CrashHandler& instance();

	void init(const QString& reportPath);

	void setReportCrashesToSystem(bool report);
	
	void setReporter(const QString& reporter);

	bool writeMinidump();

private:
	class CrashHandlerImpl;
	
	CrashHandler();

	AlignPtr<CrashHandlerImpl> impl_;
};
#endif