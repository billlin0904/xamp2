//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QNetworkDiskCache>

class NetworkDiskCache final : public QNetworkDiskCache {
public:
	explicit NetworkDiskCache(QObject* parent = nullptr);
};

