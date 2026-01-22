//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <base/uuid_class.h>
#include <base/audioformat.h>
#include <base/uuid.h>

XAMP_STREAM_NAMESPACE_BEGIN

/*
* IAudioStream is an interface for audio stream.
* 
* @note
*  This interface is not thread-safe.
*/
class XAMP_STREAM_API XAMP_NO_VTABLE IAudioStream : public IUUIDClass {
public:
    XAMP_BASE_CLASS(IAudioStream)

    /*
    * Check if the stream is a file.
    * 
    * @return true if the stream is a file, otherwise false.
    */
    [[nodiscard]] virtual bool IsFile() const noexcept = 0;

    /*
    * Close the stream.
    * 
    */
	virtual void Close() noexcept = 0;

    /*
    * Get the duration of the stream.
    * 
    * @return the duration of the stream (unit: seconds).
    */
    [[nodiscard]] virtual double GetDuration() const = 0;

    /*
    * Get audio samples.
    *
    * @param buffer The buffer to store samples.
    * @param length The length of buffer.
    *
    * @return The number of samples read.
    */
    virtual uint32_t GetSamples(void *buffer, uint32_t length) const = 0;

    /*
    * Get the format of the stream.
    * 
    * @return The format of the stream.
    */
    [[nodiscard]] virtual AudioFormat GetFormat() const = 0;

    /*
    * Seek to the specified time.
    * 
    * @param stream_time The time to seek (unit: seconds).
    */
    virtual void Seek(double stream_time) const = 0;
	
    /*
    * Get the size of a sample.
    * 
    * @return The size of a sample.
    */
    [[nodiscard]] virtual uint32_t GetSampleSize() const noexcept = 0;

    /*
    * Check if the stream is active.
    * 
    * @return true if the stream is active, otherwise false.
    */
    [[nodiscard]] virtual bool IsActive() const noexcept = 0;

    /*
    * Get file bit depth.
    *
    * @return: the file bit depth.
    */
    [[nodiscard]] virtual uint32_t GetBitDepth() const = 0;

    [[nodiscard]] virtual uint32_t GetBitRate() const = 0;

protected:
    IAudioStream() = default;
};

XAMP_STREAM_NAMESPACE_END
