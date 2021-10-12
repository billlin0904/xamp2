//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <functional>

#include <metadata/metadata.h>

namespace xamp::metadata::win32 {

class NTFSVolume;

class XAMP_METADATA_API NTFSJournal {
public:
	NTFSJournal();

	explicit NTFSJournal(std::shared_ptr<NTFSVolume> volume);

	void Open(std::wstring const& volume);

	void Traverse(DWORDLONG filref, std::function<void(std::wstring const&)> const& callback);

	void Traverse(std::function<void(std::wstring const&)> const& callback);
private:
	std::shared_ptr<NTFSVolume> volume_;
};

}
