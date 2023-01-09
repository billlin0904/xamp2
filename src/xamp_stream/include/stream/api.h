//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <map>
#include <base/stl.h>
#include <base/fs.h>
#include <base/align_ptr.h>
#include <base/encodingprofile.h>

#include <base/dsdsampleformat.h>
#include <stream/stream.h>

namespace xamp::stream {

class XAMP_STREAM_API StreamFactory {
public:
	StreamFactory() = delete;

    static AlignPtr<FileStream> MakeFileStream(DsdModes dsd_mode);

	static AlignPtr<IFileEncoder> MakeFlacEncoder();

	static AlignPtr<IFileEncoder> MakeAACEncoder();

	static AlignPtr<IFileEncoder> MakeWaveEncoder();

	static AlignPtr<IAudioProcessor> MakeEqualizer();

	static AlignPtr<IAudioProcessor> MakeCompressor();

	static AlignPtr<IAudioProcessor> MakeVolume();

	static AlignPtr<IDSPManager> MakeDSPManager();

	static AlignPtr<IAudioProcessor> MakeFader();

#ifdef XAMP_OS_WIN
	static AlignPtr<ICDDevice> MakeCDDevice(int32_t driver_letter);
#endif

	static Vector<EncodingProfile> GetAvailableEncodingProfile();
};

XAMP_STREAM_API bool IsDsdFile(Path const& path);

XAMP_STREAM_API IDsdStream* AsDsdStream(FileStream* stream) noexcept;

XAMP_STREAM_API const Vector<std::string>& GetSupportFileExtensions();

XAMP_STREAM_API OrderedMap<std::string, std::string> GetBassDLLVersion();

XAMP_STREAM_API IDsdStream* AsDsdStream(AlignPtr<FileStream> const & stream) noexcept;

XAMP_STREAM_API FileStream* AsFileStream(AlignPtr<IAudioStream> const& stream) noexcept;

XAMP_STREAM_API void LoadFFTLib();

#ifdef XAMP_OS_WIN
XAMP_STREAM_API void LoadR8brainLib();
#endif

XAMP_STREAM_API void LoadAvLib();

XAMP_STREAM_API void LoadSoxrLib();

XAMP_STREAM_API void LoadBassLib();


}
