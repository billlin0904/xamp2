//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
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

	void Open(const Path& path) override;

	std::optional<ReplayGain> GetReplayGain() override;
    
    TrackInfo Extract() override;

    std::optional<Vector<std::byte>> ReadEmbeddedCover() override;

    static HashSet<std::string> const & GetSupportFileExtensions();

    XAMP_NO_DISCARD bool IsSupported() const override;
private:
    class TaglibMetadataReaderImpl;
    ScopedPtr<TaglibMetadataReaderImpl> reader_;
};

XAMP_METADATA_NAMESPACE_END

