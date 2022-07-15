//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

class QSharedMemory;

class SingleInstanceApplication {
public:
    SingleInstanceApplication();

	~SingleInstanceApplication();

	bool attach(const QStringList& args) const;

private:
	QSharedMemory* singular_;
};
