//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <map>

#include <base/stl.h>
#include <base/fs.h>
#include <base/memory.h>
#include <base/dsdsampleformat.h>
#include <base/archivefile.h>
#include <stream/filestream.h>
#include <stream/ifileencoder.h>
#include <stream/stream.h>

XAMP_STREAM_NAMESPACE_BEGIN

struct XAMP_STREAM_API ArchiveFileStream {
    ArchiveFile archive_file;
	ScopedPtr<FileStream> file_stream;
};

class XAMP_STREAM_API StreamFactory {
public:
    StreamFactory() = delete;

    static ScopedPtr<FileStream> MakeFileStream(const Path& filePath,
        float rate = 0.0f,
        bool use_mqa_decode = false);

    // Create a file stream object based on the file path and DSD mode
    static ScopedPtr<FileStream> MakeFileStream(const Path& filePath,
        DsdModes dsdMode, 
        float rate = 0.0f,
        bool use_mqa_decode = false);

    static ScopedPtr<FileStream> MakeFileStream(ArchiveEntry archive_entry,
        DsdModes dsd_mode,
        float rate = 0.0f,
        bool use_mqa_decode = false);

	static std::expected<ArchiveFileStream, std::string> MakeArchiveFileStream(const Path& archive_path,
        const std::wstring& archive_entry_name,
        float rate = 0.0f,
        bool use_mqa_decode = false);

    // Create an AAC encoder object
    static ScopedPtr<IFileEncoder> MakeFileEncoder();

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

XAMP_STREAM_API IDsdStream* AsDsdStream(FileStream* stream) ;

XAMP_STREAM_API OrderedMap<std::string, std::string> GetBassDLLVersion();

XAMP_STREAM_API IDsdStream* AsDsdStream(ScopedPtr<FileStream> const & stream) ;

XAMP_STREAM_API FileStream* AsFileStream(ScopedPtr<IAudioStream> const& stream) ;

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
