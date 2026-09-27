//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
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
#include <bass/bassdsd.h>
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

struct BassPluginLoadDeleter final {
    static HPLUGIN invalid() ;
    static void close(HPLUGIN value);
};

struct BassStreamDeleter final {
    static HSTREAM invalid() ;
    static void close(HSTREAM value);
};

using BassPluginHandle = UniqueHandle<HPLUGIN, BassPluginLoadDeleter>;
using BassStreamHandle = UniqueHandle<HSTREAM, BassStreamDeleter>;

std::string GetBassVersion(uint32_t version);

class BassDSDLib final {
public:
    BassDSDLib();

    XAMP_DISABLE_COPY(BassDSDLib)

private:
    SharedLibraryHandle module_;

public:
    SharedLibraryFunction<HSTREAM(BOOL, void const *, QWORD, QWORD, DWORD, DWORD)> BASS_DSD_StreamCreateFile;
    XAMP_DECLARE_DLL_NAME(BASS_DSD_StreamCreateFileUser);
};

class BassMixLib final {
public:
    BassMixLib();

    XAMP_DISABLE_COPY(BassMixLib)

	std::string getName() const;
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

    std::string getName() const;

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

    std::string getName() const;

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

class BassLib final {
public:
	XAMP_DECLARE_SINGLETON_NAME()

    BassLib();

    ~BassLib();

    void loadAllPlugin();

    void freeAllPlugin();

    XAMP_ALWAYS_INLINE bool isPluginLoaded() const {
        return !plugins_.empty();
    }

    std::string getName() const;

    HashSet<std::string> getSupportFileExtensions() const;

    XAMP_DISABLE_COPY(BassLib)

	LoggerPtr logger;

    ScopedPtr<BassDSDLib> DSDLib;
    ScopedPtr<BassMixLib> MixLib;
    ScopedPtr<BassFxLib> FxLib;

#ifdef XAMP_OS_WIN
    ScopedPtr<BassCDLib> CDLib;
#endif
    void loadVersionInfo();
    OrderedMap<std::string, std::string> getPluginVersion() const;
    OrderedMap<std::string, std::string> getVersions() const;
private:
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
    XAMP_DECLARE_DLL_NAME(BASS_SampleGetChannel);
    XAMP_DECLARE_DLL_NAME(BASS_StreamCreateFileUser);
private:
    void loadPlugin(const  std::string & file_name);
};

#define LIB_BASS SharedSingleton<BassLib>::getInstance()

XAMP_STREAM_NAMESPACE_END

