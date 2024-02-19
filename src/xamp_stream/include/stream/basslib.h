//=====================================================================================================================
// Copyright (c) 2018-2024 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <bass/bass.h>

#ifdef XAMP_OS_WIN
#endif

#include <stream/bassexception.h>

#include <bass/bass_fx.h>
#include <bass/bassmix.h>
#include <bass/basscd.h>
#include <bass/bassenc.h>
#include <bass/bassenc_flac.h>
#include <base/singleton.h>
#include <base/dll.h>
#include <base/stl.h>
#include <base/unique_handle.h>
#include <base/memory.h>
#include <base/logger.h>

#include <cstdint>
#include <set>
#include <map>

XAMP_STREAM_NAMESPACE_BEGIN

inline constexpr DWORD kBassError{ 0xFFFFFFFF };

struct XAMP_STREAM_API BassPluginLoadDeleter final {
    static HPLUGIN invalid() noexcept;
    static void close(HPLUGIN value);
};

struct XAMP_STREAM_API BassStreamDeleter final {
    static HSTREAM invalid() noexcept;
    static void close(HSTREAM value);
};

using BassPluginHandle = UniqueHandle<HPLUGIN, BassPluginLoadDeleter>;
using BassStreamHandle = UniqueHandle<HSTREAM, BassStreamDeleter>;

XAMP_STREAM_API std::string GetBassVersion(uint32_t version);

class BassDSDLib final {
public:
    BassDSDLib();

    XAMP_DISABLE_COPY(BassDSDLib)

private:
    SharedLibraryHandle module_;

public:
    SharedLibraryFunction<HSTREAM(BOOL, void const *, QWORD, QWORD, DWORD, DWORD)> BASS_DSD_StreamCreateFile;
};

class BassMixLib final {
public:
    BassMixLib();

    XAMP_DISABLE_COPY(BassMixLib)

	std::string GetName() const;
private:
    SharedLibraryHandle module_;

public:
    XAMP_DECLARE_DLL_NAME(BASS_Mixer_StreamCreate);
    XAMP_DECLARE_DLL_NAME(BASS_Mixer_StreamAddChannel);
    XAMP_DECLARE_DLL_NAME(BASS_Mixer_GetVersion);
};

class BassFxLib final {
public:
    BassFxLib();

    std::string GetName() const;

    XAMP_DISABLE_COPY(BassFxLib)

private:
    SharedLibraryHandle module_;

public:
    XAMP_DECLARE_DLL_NAME(BASS_FX_TempoGetSource);
    XAMP_DECLARE_DLL_NAME(BASS_FX_TempoCreate);
    XAMP_DECLARE_DLL_NAME(BASS_FX_GetVersion);
};

#ifdef XAMP_OS_WIN
class BassCDLib final {
public:
    BassCDLib();

    std::string GetName() const;

    XAMP_DISABLE_COPY(BassCDLib)

private:
    SharedLibraryHandle module_;

public:
    // CD Driver
    XAMP_DECLARE_DLL_NAME(BASS_CD_GetInfo);
    XAMP_DECLARE_DLL_NAME(BASS_CD_GetSpeed);
    XAMP_DECLARE_DLL_NAME(BASS_CD_Door);
    XAMP_DECLARE_DLL_NAME(BASS_CD_DoorIsLocked);
    XAMP_DECLARE_DLL_NAME(BASS_CD_DoorIsOpen);
    XAMP_DECLARE_DLL_NAME(BASS_CD_SetInterface);
    XAMP_DECLARE_DLL_NAME(BASS_CD_SetOffset);
    XAMP_DECLARE_DLL_NAME(BASS_CD_SetSpeed);
    XAMP_DECLARE_DLL_NAME(BASS_CD_Release);
    // CD
    XAMP_DECLARE_DLL_NAME(BASS_CD_IsReady);
    XAMP_DECLARE_DLL_NAME(BASS_CD_GetID);
    XAMP_DECLARE_DLL_NAME(BASS_CD_GetTracks);
    XAMP_DECLARE_DLL_NAME(BASS_CD_GetTrackLength);
};
#endif

class BassFLACEncLib final{
public:
    BassFLACEncLib();

    std::string GetName() const;

    XAMP_DISABLE_COPY(BassFLACEncLib)

private:
    SharedLibraryHandle module_;

public:
#ifdef XAMP_OS_WIN
    SharedLibraryFunction<HENCODE(DWORD, const WCHAR*, DWORD, const WCHAR*)> BASS_Encode_FLAC_StartFile;
#else
    XAMP_DECLARE_DLL(BASS_Encode_FLAC_StartFile) BASS_Encode_FLAC_StartFile;
#endif
    XAMP_DECLARE_DLL_NAME(BASS_Encode_FLAC_GetVersion);
};

#ifdef XAMP_OS_WIN
class BassAACEncLib final {
public:
    BassAACEncLib();

    std::string GetName() const;

    XAMP_DISABLE_COPY(BassAACEncLib)

private:
    SharedLibraryHandle module_;

public:
    SharedLibraryFunction<HENCODE(DWORD, const WCHAR*, DWORD, const WCHAR*)> BASS_Encode_AAC_StartFile;
    SharedLibraryFunction<DWORD()> BASS_Encode_AAC_GetVersion;
};
#else
class BassCAEncLib final {
public:
    BassCAEncLib();

    std::string GetName() const;

    XAMP_DISABLE_COPY(BassCAEncLib)

private:
    SharedLibraryHandle module_;

public:
    XAMP_DECLARE_DLL_NAME(BASS_Encode_StartCAFile);
};
#endif

class BassEncLib final {
public:
    BassEncLib();

    std::string GetName() const;

    XAMP_DISABLE_COPY(BassEncLib)

private:
    SharedLibraryHandle module_;

public:
    SharedLibraryFunction<HENCODE(DWORD, void*, DWORD, const wchar_t*)> BASS_Encode_StartACMFile;
    XAMP_DECLARE_DLL_NAME(BASS_Encode_GetVersion);
    SharedLibraryFunction<DWORD (DWORD, void*, DWORD, const char*, DWORD)> BASS_Encode_GetACMFormat;
};

class BassLib final {
public:
    friend class Singleton<BassLib>;

    void Load();

    void Free();

    XAMP_ALWAYS_INLINE bool IsLoaded() const noexcept {
        return !plugins_.empty();
    }

    std::string GetName() const;

    HashSet<std::string> GetSupportFileExtensions() const;

    XAMP_DISABLE_COPY(BassLib)

	LoggerPtr logger;

    AlignPtr<BassDSDLib> DSDLib;
    AlignPtr<BassMixLib> MixLib;
    AlignPtr<BassFxLib> FxLib;
    AlignPtr<BassEncLib> EncLib;
    AlignPtr<BassFLACEncLib> FLACEncLib;

#ifdef XAMP_OS_MAC
    AlignPtr<BassCAEncLib> CAEncLib;
#endif

#ifdef XAMP_OS_WIN
    AlignPtr<BassCDLib> CDLib;
#endif
    void LoadVersionInfo();
    OrderedMap<std::string, std::string> GetPluginVersion() const;
    OrderedMap<std::string, std::string> GetVersions() const;
private:
    BassLib();

    ~BassLib();
    
    HashMap<std::string, BassPluginHandle> plugins_;
    OrderedMap<std::string, std::string> dll_versions_;
    SharedLibraryHandle module_;

public:
    XAMP_DECLARE_DLL_NAME(BASS_Init);
    XAMP_DECLARE_DLL_NAME(BASS_GetVersion);
    XAMP_DECLARE_DLL_NAME(BASS_SetConfig);
    SharedLibraryFunction<BOOL(DWORD option, const wchar_t *)> BASS_SetConfigPtr;
    SharedLibraryFunction<HPLUGIN(const char *, DWORD)> BASS_PluginLoad;
    XAMP_DECLARE_DLL_NAME(BASS_PluginGetInfo);
    XAMP_DECLARE_DLL_NAME(BASS_Free);
    SharedLibraryFunction<HSTREAM(BOOL, const void *, QWORD, QWORD, DWORD)> BASS_StreamCreateFile;
    XAMP_DECLARE_DLL_NAME(BASS_ChannelGetInfo);
    XAMP_DECLARE_DLL_NAME(BASS_StreamFree);
    XAMP_DECLARE_DLL_NAME(BASS_PluginFree);
    XAMP_DECLARE_DLL_NAME(BASS_ChannelGetData);
    XAMP_DECLARE_DLL_NAME(BASS_ChannelGetLength);
    XAMP_DECLARE_DLL_NAME(BASS_ChannelBytes2Seconds);
    XAMP_DECLARE_DLL_NAME(BASS_ChannelSeconds2Bytes);
    XAMP_DECLARE_DLL_NAME(BASS_ChannelSetPosition);
    XAMP_DECLARE_DLL_NAME(BASS_ChannelGetPosition);
    XAMP_DECLARE_DLL_NAME(BASS_ErrorGetCode);
    XAMP_DECLARE_DLL_NAME(BASS_ChannelGetAttribute);
    XAMP_DECLARE_DLL_NAME(BASS_ChannelSetAttribute);
    XAMP_DECLARE_DLL_NAME(BASS_StreamCreate);
    XAMP_DECLARE_DLL_NAME(BASS_StreamPutData);    
    XAMP_DECLARE_DLL_NAME(BASS_ChannelSetFX);
    XAMP_DECLARE_DLL_NAME(BASS_ChannelRemoveFX);
    XAMP_DECLARE_DLL_NAME(BASS_FXSetParameters);
    XAMP_DECLARE_DLL_NAME(BASS_FXGetParameters);
#ifdef XAMP_OS_MAC
    SharedLibraryFunction<HSTREAM(const char*, DWORD, DWORD, DOWNLOADPROC*, void*)> BASS_StreamCreateURL;
#else
    SharedLibraryFunction<HSTREAM(wchar_t*, DWORD, DWORD, DOWNLOADPROC*, void*)> BASS_StreamCreateURL;
#endif
    XAMP_DECLARE_DLL_NAME(BASS_StreamGetFilePosition);
    XAMP_DECLARE_DLL_NAME(BASS_ChannelIsActive);
private:
    void LoadPlugin(const  std::string & file_name);
};

#define BASS Singleton<BassLib>::GetInstance()

XAMP_STREAM_NAMESPACE_END

