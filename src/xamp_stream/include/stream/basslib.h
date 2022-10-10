//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstdint>
#include <set>
#include <map>

#include <bass/bass.h>
#include <bass/bassdsd.h>
#include <bass/bass_fx.h>
#include <bass/bassmix.h>
#include <bass/basscd.h>
#include <bass/bassenc.h>
#include <bass/bassenc_flac.h>

#ifdef XAMP_OS_WIN
#include <bass/bassenc_aac.h>
#endif

#include <base/singleton.h>
#include <base/dll.h>
#include <base/stl.h>
#include <base/exception.h>
#include <base/unique_handle.h>
#include <base/memory.h>
#include <base/memory_mapped_file.h>
#include <base/logger.h>

#include <stream/bassexception.h>

namespace xamp::stream {

inline constexpr DWORD kBassError{ 0xFFFFFFFF };

#define BassIfFailedThrow(result) \
    do {\
        if (!(result)) {\
            throw BassException();\
        }\
    } while (false)

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
    ModuleHandle module_;

public:
    DllFunction<HSTREAM(BOOL, void const *, QWORD, QWORD, DWORD, DWORD)> BASS_DSD_StreamCreateFile;
};

class BassMixLib final {
public:
    BassMixLib();

    XAMP_DISABLE_COPY(BassMixLib)

	std::string GetName() const;
private:
    ModuleHandle module_;

public:
    XAMP_DECLARE_DLL(BASS_Mixer_StreamCreate) BASS_Mixer_StreamCreate;
    XAMP_DECLARE_DLL(BASS_Mixer_StreamAddChannel) BASS_Mixer_StreamAddChannel;
    XAMP_DECLARE_DLL(BASS_Mixer_GetVersion) BASS_Mixer_GetVersion;
};

class BassFxLib final {
public:
    BassFxLib();

    std::string GetName() const;

    XAMP_DISABLE_COPY(BassFxLib)

private:
    ModuleHandle module_;

public:
    XAMP_DECLARE_DLL(BASS_FX_TempoGetSource) BASS_FX_TempoGetSource;
    XAMP_DECLARE_DLL(BASS_FX_TempoCreate) BASS_FX_TempoCreate;
    XAMP_DECLARE_DLL(BASS_FX_GetVersion) BASS_FX_GetVersion;
};

#ifdef XAMP_OS_WIN
class BassCDLib final {
public:
    BassCDLib();

    std::string GetName() const;

    XAMP_DISABLE_COPY(BassCDLib)

private:
    ModuleHandle module_;

public:
    // CD Driver
    XAMP_DECLARE_DLL(BASS_CD_GetInfo) BASS_CD_GetInfo;
    XAMP_DECLARE_DLL(BASS_CD_GetSpeed) BASS_CD_GetSpeed;
    XAMP_DECLARE_DLL(BASS_CD_Door) BASS_CD_Door;
    XAMP_DECLARE_DLL(BASS_CD_DoorIsLocked) BASS_CD_DoorIsLocked;
    XAMP_DECLARE_DLL(BASS_CD_DoorIsOpen) BASS_CD_DoorIsOpen;
    XAMP_DECLARE_DLL(BASS_CD_SetInterface) BASS_CD_SetInterface;
    XAMP_DECLARE_DLL(BASS_CD_SetOffset) BASS_CD_SetOffset;
    XAMP_DECLARE_DLL(BASS_CD_SetSpeed) BASS_CD_SetSpeed;
    XAMP_DECLARE_DLL(BASS_CD_Release) BASS_CD_Release;
    // CD
    XAMP_DECLARE_DLL(BASS_CD_IsReady) BASS_CD_IsReady;
    XAMP_DECLARE_DLL(BASS_CD_GetID) BASS_CD_GetID;
    XAMP_DECLARE_DLL(BASS_CD_GetTracks) BASS_CD_GetTracks;
    XAMP_DECLARE_DLL(BASS_CD_GetTrackLength) BASS_CD_GetTrackLength;
};
#endif

class BassFLACEncLib final{
public:
    BassFLACEncLib();

    std::string GetName() const;

    XAMP_DISABLE_COPY(BassFLACEncLib)

private:
    ModuleHandle module_;

public:
#ifdef XAMP_OS_WIN
    DllFunction<HENCODE(DWORD, const WCHAR*, DWORD, const WCHAR*)> BASS_Encode_FLAC_StartFile;
#else
    XAMP_DECLARE_DLL(BASS_Encode_FLAC_StartFile) BASS_Encode_FLAC_StartFile;
#endif
    XAMP_DECLARE_DLL(BASS_Encode_FLAC_GetVersion) BASS_Encode_FLAC_GetVersion;
};

class BassAACEncLib final {
public:
    BassAACEncLib();

    std::string GetName() const;

    XAMP_DISABLE_COPY(BassAACEncLib)

private:
    ModuleHandle module_;

public:
#ifdef XAMP_OS_WIN
    DllFunction<HENCODE(DWORD, const WCHAR*, DWORD, const WCHAR*)> BASS_Encode_AAC_StartFile;
#else
    XAMP_DECLARE_DLL(BASS_Encode_FLAC_StartFile) BASS_Encode_AAC_StartFile;
#endif
};

class BassEncLib final {
public:
    BassEncLib();

    std::string GetName() const;

    XAMP_DISABLE_COPY(BassEncLib)

private:
    ModuleHandle module_;

public:
    DllFunction<HENCODE(DWORD, void*, DWORD, const wchar_t*)> BASS_Encode_StartACMFile;
    XAMP_DECLARE_DLL(BASS_Encode_GetVersion) BASS_Encode_GetVersion;
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

    AlignPtr<BassDSDLib> DSDLib;
    AlignPtr<BassMixLib> MixLib;
    AlignPtr<BassFxLib> FxLib;
    AlignPtr<BassEncLib> EncLib;
    AlignPtr<BassFLACEncLib> FLACEncLib;
    AlignPtr<BassAACEncLib> AACEncLib;

#ifdef XAMP_OS_WIN
    AlignPtr<BassCDLib> CDLib;
#endif
    void LoadVersionInfo();
    std::map<std::string, std::string> GetPluginVersion() const;
    std::map<std::string, std::string> GetVersions() const;

private:
    BassLib();
    
    HashMap<std::string, BassPluginHandle> plugins_;
    std::map<std::string, std::string> dll_versions_;
    ModuleHandle module_;

public:
    XAMP_DECLARE_DLL(BASS_Init) BASS_Init;
    XAMP_DECLARE_DLL(BASS_GetVersion) BASS_GetVersion;
    XAMP_DECLARE_DLL(BASS_SetConfig) BASS_SetConfig;
    DllFunction<BOOL(DWORD option, const wchar_t *)> BASS_SetConfigPtr;
    DllFunction<HPLUGIN(const char *, DWORD)> BASS_PluginLoad;
    XAMP_DECLARE_DLL(BASS_PluginGetInfo) BASS_PluginGetInfo;
    XAMP_DECLARE_DLL(BASS_Free) BASS_Free;
    DllFunction<HSTREAM(BOOL, const void *, QWORD, QWORD, DWORD)> BASS_StreamCreateFile;
    XAMP_DECLARE_DLL(BASS_ChannelGetInfo) BASS_ChannelGetInfo;
    XAMP_DECLARE_DLL(BASS_StreamFree) BASS_StreamFree;
    XAMP_DECLARE_DLL(BASS_PluginFree) BASS_PluginFree;
    XAMP_DECLARE_DLL(BASS_ChannelGetData) BASS_ChannelGetData;
    XAMP_DECLARE_DLL(BASS_ChannelGetLength) BASS_ChannelGetLength;
    XAMP_DECLARE_DLL(BASS_ChannelBytes2Seconds) BASS_ChannelBytes2Seconds;
    XAMP_DECLARE_DLL(BASS_ChannelSeconds2Bytes) BASS_ChannelSeconds2Bytes;
    XAMP_DECLARE_DLL(BASS_ChannelSetPosition) BASS_ChannelSetPosition;
    XAMP_DECLARE_DLL(BASS_ChannelGetPosition) BASS_ChannelGetPosition;
    XAMP_DECLARE_DLL(BASS_ErrorGetCode) BASS_ErrorGetCode;
    XAMP_DECLARE_DLL(BASS_ChannelGetAttribute) BASS_ChannelGetAttribute;
    XAMP_DECLARE_DLL(BASS_ChannelSetAttribute) BASS_ChannelSetAttribute;
    XAMP_DECLARE_DLL(BASS_StreamCreate) BASS_StreamCreate;
    XAMP_DECLARE_DLL(BASS_StreamPutData) BASS_StreamPutData;    
    XAMP_DECLARE_DLL(BASS_ChannelSetFX) BASS_ChannelSetFX;
    XAMP_DECLARE_DLL(BASS_ChannelRemoveFX) BASS_ChannelRemoveFX;
    XAMP_DECLARE_DLL(BASS_FXSetParameters) BASS_FXSetParameters;
    XAMP_DECLARE_DLL(BASS_FXGetParameters) BASS_FXGetParameters;
#ifdef XAMP_OS_MAC
    DllFunction<HSTREAM(const char*, DWORD, DWORD, DOWNLOADPROC*, void*)> BASS_StreamCreateURL;
#else
    DllFunction<HSTREAM(wchar_t*, DWORD, DWORD, DOWNLOADPROC*, void*)> BASS_StreamCreateURL;
#endif
    XAMP_DECLARE_DLL(BASS_StreamGetFilePosition) BASS_StreamGetFilePosition;
    XAMP_DECLARE_DLL(BASS_ChannelIsActive) BASS_ChannelIsActive;
private:
    void LoadPlugin(std::string const & file_name);
};

#define BASS Singleton<BassLib>::GetInstance()

}

