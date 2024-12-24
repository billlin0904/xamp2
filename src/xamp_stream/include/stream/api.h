//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <map>

#include <base/stl.h>
#include <base/fs.h>
#include <base/memory.h>
#include <base/dsdsampleformat.h>

#include <stream/ifileencoder.h>
#include <stream/stream.h>

XAMP_STREAM_NAMESPACE_BEGIN


class XAMP_STREAM_API StreamFactory {
public:
    StreamFactory() = delete;

    // Create a file stream object based on the file path and DSD mode
    static ScopedPtr<FileStream> MakeFileStream(const Path &filePath, DsdModes dsdMode);

    // Create an AAC encoder object
    static ScopedPtr<IFileEncoder> MakeM4AEncoder();

    // Create an equalizer audio processor object
    static ScopedPtr<IAudioProcessor> MakeEqualizer();

    #ifdef XAMP_OS_WIN
    // Create an super EQ equalizer audio processor object
    static ScopedPtr<IAudioProcessor> MakeSuperEqEqualizer();
    #endif

    // Create a parametric equalizer audio processor object
    static ScopedPtr<IAudioProcessor> MakeParametricEq();

    // Create a compressor audio processor object
    static ScopedPtr<IAudioProcessor> MakeCompressor();

    // Create a DSP manager object
    static ScopedPtr<IDSPManager> MakeDSPManager();

    // Create a fader audio processor object
    static ScopedPtr<IAudioProcessor> MakeFader();

    // Create a CD device object (specific to Windows OS)
    #ifdef XAMP_OS_WIN
    static ScopedPtr<ICDDevice> MakeCDDevice(int32_t driverLetter);
    #endif
};

XAMP_STREAM_API bool IsDsdFile(Path const& path);

XAMP_STREAM_API IDsdStream* AsDsdStream(FileStream* stream) noexcept;

XAMP_STREAM_API OrderedMap<std::string, std::string> GetBassDLLVersion();

XAMP_STREAM_API IDsdStream* AsDsdStream(ScopedPtr<FileStream> const & stream) noexcept;

XAMP_STREAM_API FileStream* AsFileStream(ScopedPtr<IAudioStream> const& stream) noexcept;

XAMP_STREAM_API ScopedPtr<FileStream> MakeFileStream(const Path& file_path, DsdModes dsd_mode);

XAMP_STREAM_API std::shared_ptr<IIoContext> MakFileEncodeWriter(const Path& file_path);

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

XAMP_STREAM_NAMESPACE_END
