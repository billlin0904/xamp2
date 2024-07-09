//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <map>
#include <base/stl.h>
#include <base/fs.h>
#include <base/memory.h>
#include <base/encodingprofile.h>

#include <base/dsdsampleformat.h>
#include <stream/stream.h>

XAMP_STREAM_NAMESPACE_BEGIN

class XAMP_STREAM_API StreamFactory {
public:
    StreamFactory() = delete;

    // Create a file stream object based on the file path and DSD mode
    static AlignPtr<FileStream> MakeFileStream(const Path &filePath, DsdModes dsdMode);

    // Create a FLAC encoder object
    static AlignPtr<IFileEncoder> MakeFlacEncoder();

    // Create an ALAC encoder object
    static AlignPtr<IFileEncoder> MakeAlacEncoder();

    // Create an AAC encoder object
    static AlignPtr<IFileEncoder> MakeAACEncoder();

    // Create a WAVE encoder object
    static AlignPtr<IFileEncoder> MakeWaveEncoder();

    // Create an equalizer audio processor object
    static AlignPtr<IAudioProcessor> MakeEqualizer();

    #ifdef XAMP_OS_WIN
    // Create an super EQ equalizer audio processor object
    static AlignPtr<IAudioProcessor> MakeSuperEqEqualizer();
    #endif

    // Create a parametric equalizer audio processor object
    static AlignPtr<IAudioProcessor> MakeParametricEq();

    // Create a compressor audio processor object
    static AlignPtr<IAudioProcessor> MakeCompressor();

    // Create a volume audio processor object
    static AlignPtr<IAudioProcessor> MakeVolume();

    // Create a DSP manager object
    static AlignPtr<IDSPManager> MakeDSPManager();

    // Create a fader audio processor object
    static AlignPtr<IAudioProcessor> MakeFader();

    // Create a CD device object (specific to Windows OS)
    #ifdef XAMP_OS_WIN
    static AlignPtr<ICDDevice> MakeCDDevice(int32_t driverLetter);
    #endif

    // Get a vector of available encoding profiles
    static Vector<EncodingProfile> GetAvailableEncodingProfile();
};

XAMP_STREAM_API bool IsDsdFile(Path const& path);

XAMP_STREAM_API IDsdStream* AsDsdStream(FileStream* stream) noexcept;

XAMP_STREAM_API OrderedMap<std::string, std::string> GetBassDLLVersion();

XAMP_STREAM_API IDsdStream* AsDsdStream(AlignPtr<FileStream> const & stream) noexcept;

XAMP_STREAM_API FileStream* AsFileStream(AlignPtr<IAudioStream> const& stream) noexcept;

XAMP_STREAM_API AlignPtr<FileStream> MakeFileStream(const Path& file_path, DsdModes dsd_mode);

XAMP_STREAM_API void LoadFFTLib();

#ifdef XAMP_OS_WIN
XAMP_STREAM_API void LoadR8brainLib();
XAMP_STREAM_API void LoadMBDiscIdLib();
#endif

XAMP_STREAM_API void LoadAvLib();

XAMP_STREAM_API void FreeAvLib();

XAMP_STREAM_API void LoadSoxrLib();

XAMP_STREAM_API void LoadSrcLib();

XAMP_STREAM_API void LoadBassLib();

XAMP_STREAM_API void LoadEbur128Lib();

XAMP_STREAM_NAMESPACE_END
