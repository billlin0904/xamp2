//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/str_utilts.h>
#include <widget/widget_shared_global.h>

struct Version {
	int32_t major_part{ -1 };
	int32_t minor_part{ -1 };
	int32_t revision_part{ -1 };

	bool operator > (const Version& version) const noexcept {
		return major_part > version.major_part
			|| minor_part > version.minor_part
			|| revision_part > version.revision_part;
	}
};

XAMP_WIDGET_SHARED_EXPORT extern const Version kApplicationVersionValue;
XAMP_WIDGET_SHARED_EXPORT extern const ConstLatin1String kApplicationName;
XAMP_WIDGET_SHARED_EXPORT extern const ConstLatin1String kApplicationTitle;
XAMP_WIDGET_SHARED_EXPORT extern const ConstLatin1String kApplicationVersion;
XAMP_WIDGET_SHARED_EXPORT extern const ConstLatin1String kDefaultCharset;
XAMP_WIDGET_SHARED_EXPORT extern const ConstLatin1String kDefaultUserAgent;

XAMP_WIDGET_SHARED_EXPORT bool ParseVersion(const QString &s, Version &version);