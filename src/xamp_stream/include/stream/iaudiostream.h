//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <base/pcm.h>
#include <optional>
#include <stdexcept>
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
    [[nodiscard]] virtual std::optional<xamp::pcm::Format> integerPcmFormat() const { return std::nullopt; }
    xamp::pcm::Block readPcm(std::span<std::byte> storage) const {
        const auto format = integerPcmFormat();
        if (!format || !xamp::pcm::valid(*format) || format->layout != xamp::pcm::Layout::Interleaved)
            throw std::runtime_error("Stream does not expose interleaved integer PCM");
        const auto frames = storage.size()/format->frameBytes();
        if (frames > UINT32_MAX/format->channels) throw std::length_error("PCM block too large");
        const auto samples = getSamples(storage.data(),static_cast<uint32_t>(frames*format->channels));
        if (samples > frames*format->channels || samples%format->channels)
            throw std::runtime_error("Decoder returned an incomplete PCM frame");
        return {storage.first(samples*format->sampleBytes()), samples/format->channels, *format};
    }


    /*
    * Check if the stream is a file.
    * 
    * @return true if the stream is a file, otherwise false.
    */
    [[nodiscard]] virtual bool isFile() const = 0;

    /*
    * close the stream.
    * 
    */
	virtual void close() = 0;

    /*
    * Get the duration of the stream.
    * 
    * @return the duration of the stream (unit: seconds).
    */
    [[nodiscard]] virtual double getDuration() const = 0;

    /*
    * Get audio samples.
    *
    * @param buffer The buffer to store samples.
    * @param length The length of buffer.
    *
    * @return The number of samples read.
    */
    virtual uint32_t getSamples(void *buffer, uint32_t length) const = 0;

    /*
    * Get the format of the stream.
    * 
    * @return The format of the stream.
    */
    [[nodiscard]] virtual AudioFormat getFormat() const = 0;

    /*
    * seek to the specified time.
    * 
    * @param stream_time The time to seek (unit: seconds).
    */
    virtual void seek(double stream_time) const = 0;
	
    /*
    * Get the size of a sample.
    * 
    * @return The size of a sample.
    */
    [[nodiscard]] virtual uint32_t getSampleSize() const = 0;

    /*
    * Check if the stream is active.
    * 
    * @return true if the stream is active, otherwise false.
    */
    [[nodiscard]] virtual bool isActive() const = 0;

    /*
    * Get file bit depth.
    *
    * @return: the file bit depth.
    */
    [[nodiscard]] virtual uint32_t getBitDepth() const = 0;

    [[nodiscard]] virtual uint32_t getBitRate() const = 0;

protected:
    IAudioStream() = default;
};

XAMP_STREAM_NAMESPACE_END
