//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/filestream.h>
#include <stream/idsdstream.h>
#include <base/memory.h>

// Generated stereo DSD: packed MSB-first bytes for Native DSD, or an integer
// DoP carrier. integerPcmFormat is available only for the DoP carrier.
XAMP_STREAM_NAMESPACE_BEGIN

class XAMP_STREAM_API Pcm2DsdFileStream final : public FileStream, public IDsdStream {
    XAMP_DECLARE_MAKE_CLASS_UUID(Pcm2DsdFileStream, "D5D25498-85EA-4F21-B3E0-358A8190F702")
public:
    XAMP_DECLARE_UUID_CLASS_DESC(Pcm2DsdFileStream, "PCM2DSD stream")

    Pcm2DsdFileStream(ScopedPtr<FileStream> source,
        uint32_t multiplier,
        double gain_db,
        bool dither);

    Pcm2DsdFileStream(ScopedPtr<FileStream> source,
        uint32_t multiplier,
        double gain_db,
        bool dither,
        uint32_t filter,
        uint32_t modulator,
        uint32_t block_frames);

    ~Pcm2DsdFileStream() override;

    void openFile(const Path& path) override;

    void open(ArchiveEntry entry) override;

    void close() override;

    [[nodiscard]] bool endOfStream() const override;

    [[nodiscard]] bool isActive() const override;

    [[nodiscard]] double getDuration() const override;

    [[nodiscard]] uint32_t getSamples(void* buffer, uint32_t length) const override;

    [[nodiscard]] AudioFormat getFormat() const override;

    [[nodiscard]] std::optional<pcm::Format> integerPcmFormat() const override;

    void seek(double seconds) const override;

    [[nodiscard]] uint32_t getSampleSize() const override;

    [[nodiscard]] uint32_t getBitDepth() const override;

    [[nodiscard]] uint32_t getBitRate() const override;

    void setDSDMode(DsdModes mode) override;

    [[nodiscard]] DsdModes getDsdMode() const override;

    [[nodiscard]] uint32_t getDsdSampleRate() const override;

    [[nodiscard]] DsdFormat getDsdFormat() const override {
        return DsdFormat::DSD_INT8MSB;
    }
    void setDsdToPcmSampleRate(uint32_t) override;

    [[nodiscard]] uint32_t getDsdSpeed() const override;
    // IDsdStream describes the generated DSD stream; original file is PCM.
    [[nodiscard]] bool isDsdFile() const override {
        return true;
    }
    [[nodiscard]] bool supportDOP() const override {
        return true;
    }
    [[nodiscard]] bool supportDOP_AA() const override {
        return false;
    }
    [[nodiscard]] bool supportNativeSD() const override {
        return true;
    }

private:
    class Pcm2DsdFileStreamImpl;
    ScopedPtr<Pcm2DsdFileStreamImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END
