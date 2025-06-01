//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
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

    virtual void Open(const Path& path) = 0;

    /*
    * Write track information to file.
    * 
    * @param[in] path file path.
    * @param[in] track_info track info.
    */
    virtual void Write(const TrackInfo& track_info) = 0;

    /*
    * Write artist to file.
    * 
    * @param[in] path file path.
    * @param[in] artist.
    */
    virtual void WriteArtist(const std::wstring& artist) = 0;

    /*
    * Write album to file.
    * 
    * @param[in] path file path.
    * @param[in] album.
    */
    virtual void WriteAlbum(const std::wstring& album) = 0;

    /*
    * Write title to file.
    * 
    * @param[in] path file path.
    * @param[in] title.
    */
    virtual void WriteTitle(const std::wstring& title) = 0;
    
    /*
    * Write track number to file.
    * 
    * @param[in] path file path.
    * @param[in] track number.
    */
    virtual void WriteTrack(uint32_t track) = 0;

    /*
    * Write disc number to file.
    * 
    * @param[in] path file path.
    * @param[in] genre.
    */
    virtual void WriteGenre(const std::wstring& genre) = 0;

    /*
    * Write year to file.
    * 
    * @param[in] path file path.
    * @param[in] comment.
    */
    virtual void WriteComment(const std::wstring& comment) = 0;

    /*
    * Write year to file.
    * 
    * @param[in] path file path.
    * @param[in] year.
    */
    virtual void WriteYear(uint32_t year) = 0;

    /*
    * Write ReplayGain to file.
    * 
    * @param[in] path file path.
    * @param[in] replay_gain replay gain.
    */
    virtual void WriteReplayGain(const ReplayGain& replay_gain) = 0;

    /*
    * Write embedded cover to file.
    * 
    * @param[in] path file path.
    * @param[in] image data.
    */
    virtual void WriteEmbeddedCover(const std::vector<uint8_t> & image) const = 0;

    /*
    * Remove embedded cover from file.
    * 
    * @param[in] path file path.
    */
    virtual void RemoveEmbeddedCover() = 0;

    /*
    * Check file is supported.
    * 
    * @param[in] path file path.
    */
    [[nodiscard]] virtual bool CanWriteEmbeddedCover() const = 0;
protected:
    IMetadataWriter() = default;
};

XAMP_METADATA_NAMESPACE_END
