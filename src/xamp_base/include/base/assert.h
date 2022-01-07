//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

namespace xamp::base {

enum class AssertHandleTypes {
	ASSERT_DEBUG,
	ASSERT_GIVE_UP,
	ASSERT_IGNORE,
};

class XAMP_BASE_API Asserter {
public:
	virtual ~Asserter() = default;

	virtual bool Handle(const char* file, int line, const char* expr) const;

	virtual AssertHandleTypes AskUser(const char* file, int line, const char* expr) const;

	static Asserter MakeAsserter(bool flag);

	static void DebugTrap();

protected:
	Asserter(bool hold);
	bool hold_;
};

#ifdef _DEBUG
#define XAMP_ASSERT(expr) \
	if (!(expr)) {\
		using xamp::base::Asserter;\
		if (Asserter::MakeAsserter(false).Handle(__FILE__, __LINE__, #expr)) {\
			Asserter::DebugTrap(); \
		}\
	} else {\
		((void)0);\
	}
#else
#define XAMP_ASSERT(expr) ((void)0)
#endif
}

