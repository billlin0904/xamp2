//=====================================================================================================================
// Copyright (c) 2018-2024 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/fs.h>
#include <base/stl.h>
#include <base/trackinfo.h>
#include <metadata/metadata.h>

XAMP_METADATA_NAMESPACE_BEGIN

/*
* IMetadataWriter is a interface for writing metadata to file.
* 
*/
class XAMP_METADATA_API XAMP_NO_VTABLE IMetadataWriter {
public:
    XAMP_BASE_CLASS(IMetadataWriter)

    /*
    * Check file is read only.
    * 
    * @param[in] path file path.
    * @return bool
    */
    [[nodiscard]] virtual bool IsFileReadOnly(const Path & path) const = 0;
    
    /*
    * Write track information to file.
    * 
    * @param[in] path file path.
    * @param[in] track_info track info.
    */
    virtual void Write(const Path &path, const TrackInfo& track_info) = 0;

    virtual void WriteArtist(const Path& path, const std::wstring& artist) = 0;

    virtual void WriteAlbum(const Path& path, const std::wstring& album) = 0;

    virtual void WriteTitle(const Path& path, const std::wstring& title) = 0;

    virtual void WriteTrack(const Path& path, uint32_t track) = 0;

    virtual void WriteGenre(const Path& path, const std::wstring& genre) = 0;

    virtual void WriteComment(const Path& path, const std::wstring& comment) = 0;

    virtual void WriteYear(const Path& path, uint32_t year) = 0;

    /*
    * Write ReplayGain to file.
    * 
    * @param[in] path file path.
    * @param[in] replay_gain replay gain.
    */
    virtual void WriteReplayGain(const Path& path, const ReplayGain& replay_gain) = 0;

    /*
    * Write embedded cover to file.
    * 
    * @param[in] path file path.
    * @param[in] image image data.
    */
    virtual void WriteEmbeddedCover(const Path& path, const Vector<uint8_t> & image) const = 0;

    virtual void RemoveEmbeddedCover(const Path& path) = 0;

    [[nodiscard]] virtual bool CanWriteEmbeddedCover(const Path& path) const = 0;
protected:
    IMetadataWriter() = default;
};

XAMP_METADATA_NAMESPACE_END
