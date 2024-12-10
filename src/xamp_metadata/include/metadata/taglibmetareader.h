//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <metadata/imetadatareader.h>

#include <base/stl.h>
#include <base/memory.h>
#include <base/pimplptr.h>

XAMP_METADATA_NAMESPACE_BEGIN

class TaglibMetadataReader final : public IMetadataReader {
public:
    TaglibMetadataReader();

    XAMP_PIMPL(TaglibMetadataReader)

	std::optional<ReplayGain> GetReplayGain(const Path& path) override;
    
    TrackInfo Extract(const Path& path) override;

    std::optional<Vector<uint8_t>> ReadEmbeddedCover(const Path& path) override;

    static HashSet<std::string> const & GetSupportFileExtensions();

    XAMP_NO_DISCARD bool IsSupported(const Path& path) const override;
private:
    class TaglibMetadataReaderImpl;
    ScopedPtr<TaglibMetadataReaderImpl> reader_;
};

XAMP_METADATA_NAMESPACE_END

