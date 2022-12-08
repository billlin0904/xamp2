//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/uuid.h>

template <typename T>
struct UuidOf {
	static inline
	const xamp::base::Uuid& Id() {
		return xamp::base::Uuid::kNullUuid;
	}
};

#define XAMP_MAKE_CLASS_UUID(ClassName, UuidString)\
template <>                                 \
struct UuidOf<ClassName> {                  \
	static inline							\
	const xamp::base::Uuid& Id() {			\
		static const xamp::base::Uuid id(	\
		UuidString);						\
		return id;							\
	}										\
};
