//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/fs.h>
#include <base/stl.h>
#include <base/trackinfo.h>
#include <metadata/metadata.h>

XAMP_METADATA_NAMESPACE_BEGIN

/*
* IMetadataWriter is an interface for writing metadata to file.
* 
*/
class XAMP_METADATA_API XAMP_NO_VTABLE IMetadataWriter {
public:
    XAMP_BASE_CLASS(IMetadataWriter)

    virtual void open(const Path& path) = 0;

    /*
    * write track information to file.
    * 
    * @param[in] path file path.
    * @param[in] track_info track info.
    */
    virtual void write(const TrackInfo& track_info) = 0;

    /*
    * write artist to file.
    * 
    * @param[in] path file path.
    * @param[in] artist.
    */
    virtual void writeArtist(const std::wstring& artist) = 0;

    /*
    * write album to file.
    * 
    * @param[in] path file path.
    * @param[in] album.
    */
    virtual void writeAlbum(const std::wstring& album) = 0;

    /*
    * write title to file.
    * 
    * @param[in] path file path.
    * @param[in] title.
    */
    virtual void writeTitle(const std::wstring& title) = 0;
    
    /*
    * write track number to file.
    * 
    * @param[in] path file path.
    * @param[in] track number.
    */
    virtual void writeTrack(uint32_t track) = 0;

    /*
    * write disc number to file.
    * 
    * @param[in] path file path.
    * @param[in] genre.
    */
    virtual void writeGenre(const std::wstring& genre) = 0;

    /*
    * write year to file.
    * 
    * @param[in] path file path.
    * @param[in] comment.
    */
    virtual void writeComment(const std::wstring& comment) = 0;

    /*
    * write year to file.
    * 
    * @param[in] path file path.
    * @param[in] year.
    */
    virtual void writeYear(uint32_t year) = 0;

    /*
    * write ReplayGain to file.
    * 
    * @param[in] path file path.
    * @param[in] replay_gain replay gain.
    */
    virtual void writeReplayGain(const ReplayGain& replay_gain) = 0;

    /*
    * write embedded cover to file.
    * 
    * @param[in] path file path.
    * @param[in] image data.
    */
    virtual void writeEmbeddedCover(const std::vector<uint8_t> & image) const = 0;

    /*
    * Remove embedded cover from file.
    * 
    * @param[in] path file path.
    */
    virtual void removeEmbeddedCover() = 0;

    /*
    * Check file is supported.
    * 
    * @param[in] path file path.
    */
    [[nodiscard]] virtual bool canWriteEmbeddedCover() const = 0;
protected:
    IMetadataWriter() = default;
};

XAMP_METADATA_NAMESPACE_END
