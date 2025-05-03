//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QByteArray>
#include <vector>

#include <widget/widget_shared.h>

QByteArray gzipDecompress(const QByteArray& data);

bool decompress(const uint8_t* in_data, size_t in_size, std::string& out_data);

#if XAMP_OS_WIN
class ZipFileReader {
public:
	ZipFileReader();

	XAMP_PIMPL(ZipFileReader)

	std::vector<std::wstring> openFile(const std::wstring& file);

private:
	class ZipFileReaderImpl;
	ScopedPtr<ZipFileReaderImpl> impl_;
};
#endif
