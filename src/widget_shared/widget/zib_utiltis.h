//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QByteArray>
#include <vector>

#include <widget/widget_shared.h>

QByteArray GzipDecompress(const QByteArray& data);

class ZipFileReader {
public:
	ZipFileReader();

	XAMP_PIMPL(ZipFileReader)

	std::vector<std::wstring> openFile(const std::wstring& file);

private:
	class ZipFileReaderImpl;
	AlignPtr<ZipFileReaderImpl> impl_;
};
