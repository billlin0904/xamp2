//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <metadata/imetadatareader.h>

#include <base/stl.h>
#include <base/align_ptr.h>
#include <base/pimplptr.h>

namespace xamp::metadata {

class TaglibMetadataReader final : public IMetadataReader {
public:
    TaglibMetadataReader();

    XAMP_PIMPL(TaglibMetadataReader)

	std::optional<ReplayGain> GetReplayGain(const Path& path) override;
    
    TrackInfo Extract(Path const &path) override;

    const Vector<uint8_t>& GetEmbeddedCover(Path const &path) override;

    [[nodiscard]] HashSet<std::string> const & GetSupportFileExtensions() const override;

    [[nodiscard]] bool IsSupported(Path const & path) const override;
private:
    class TaglibMetadataReaderImpl;
    PimplPtr<TaglibMetadataReaderImpl> reader_;
};

}

