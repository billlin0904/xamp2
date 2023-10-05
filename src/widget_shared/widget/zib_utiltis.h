//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QByteArray>

#include <fstream>
#include <vector>
#include <map>

#include <widget/widget_shared.h>

bool IsLoadZib();

QByteArray GzipDecompress(const QByteArray& data);

class ZipFileReader {
public:
	ZipFileReader();

	XAMP_PIMPL(ZipFileReader)

	std::vector<std::wstring> OpenFile(const std::wstring& file);

private:
	class ZipFileReaderImpl;
	PimplPtr<ZipFileReaderImpl> impl_;
};
