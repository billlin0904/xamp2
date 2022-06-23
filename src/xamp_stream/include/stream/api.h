//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <map>
#include <base/stl.h>
#include <base/align_ptr.h>

#include <stream/stream.h>

namespace xamp::stream {

class XAMP_STREAM_API MediaStreamFactory {
public:
	MediaStreamFactory() = delete;

	static AlignPtr<IAudioStream> MakeAudioStream();

	static AlignPtr<IFileEncoder> MakeFlacEncoder();

	static AlignPtr<IFileEncoder> MakeWaveEncoder();

	static AlignPtr<IAudioProcessor> MakeEqualizer();

	static AlignPtr<IAudioProcessor> MakeCompressor();

	static AlignPtr<IAudioProcessor> MakeVolume();

	static AlignPtr<IDSPManager> MakeDSPManager();

#ifdef XAMP_OS_WIN
	static AlignPtr<ICDDevice> MakeCDDevice(int32_t driver_letter);
#endif
};

XAMP_STREAM_API bool TestDsdFileFormatStd(std::wstring const& file_path);

XAMP_STREAM_API IDsdStream* AsDsdStream(IAudioStream* stream) noexcept;

XAMP_STREAM_API HashSet<std::string> const& GetSupportFileExtensions();

XAMP_STREAM_API std::map<std::string, std::string> GetBassDLLVersion();

XAMP_STREAM_API IDsdStream* AsDsdStream(AlignPtr<IAudioStream> const & stream) noexcept;

XAMP_STREAM_API FileStream* AsFileStream(AlignPtr<IAudioStream> const& stream) noexcept;

#ifdef XAMP_OS_WIN
XAMP_STREAM_API void LoadFFTLib();
XAMP_STREAM_API void LoadR8brainLib();
#endif

XAMP_STREAM_API void LoadSoxrLib();

XAMP_STREAM_API void LoadBassLib();

XAMP_STREAM_API void FreeBassLib();

}
