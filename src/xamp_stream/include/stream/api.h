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

    static ScopedPtr<FileStream> makeFileStream(const Path& filePath,
        bool use_mqa_decode = false);

    // create a file stream object based on the file path and DSD mode
    static ScopedPtr<FileStream> makeFileStream(const Path& filePath,
        DsdModes dsdMode, 
        bool use_mqa_decode = false);

    static ScopedPtr<FileStream> makeFileStream(ArchiveEntry archive_entry,
        DsdModes dsd_mode);

	static std::expected<ArchiveFileStream, std::string> makeArchiveFileStream(const Path& archive_path,
        const std::wstring& archive_entry_name);

    // create an AAC encoder object
    static ScopedPtr<IFileEncoder> makeFileEncoder();

    // create a parametric equalizer audio processor object
    static ScopedPtr<IAudioProcessor> makeParametricEq();

    // create a DSP manager object
    static ScopedPtr<IDSPManager> makeDSPManager();

    // create a CD device object (specific to Windows OS)
    #ifdef XAMP_OS_WIN
    static ScopedPtr<ICDDevice> makeCDDevice(int32_t driverLetter);
    #endif
};

XAMP_STREAM_API bool isDsdFile(Path const& path);

XAMP_STREAM_API IDsdStream* asDsdStream(FileStream* stream) ;

XAMP_STREAM_API OrderedMap<std::string, std::string> getBassDLLVersion();

XAMP_STREAM_API IDsdStream* asDsdStream(ScopedPtr<FileStream> const & stream) ;

XAMP_STREAM_API FileStream* asFileStream(ScopedPtr<IAudioStream> const& stream) ;

#ifdef XAMP_OS_WIN
XAMP_STREAM_API void loadR8BrainLib();
XAMP_STREAM_API void loadMBDiscIdLib();
#endif

XAMP_STREAM_API void loadAvLib();

XAMP_STREAM_API void freeAvLib();

XAMP_STREAM_API void loadSoxrLib();

XAMP_STREAM_API void loadSrcLib();

XAMP_STREAM_API void loadBassLib();

XAMP_STREAM_API void loadMqaLib();

XAMP_STREAM_NAMESPACE_END
