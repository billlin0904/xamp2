//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstdint>

#include <bass/bass.h>
#include <bass/bassdsd.h>
#include <bass/bass_fx.h>

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
inline constexpr int32_t kPcmSampleRate441 { 44100 };

#define BassIfFailedThrow(result) \
    do {\
        if (!(result)) {\
            throw BassException();\
        }\
    } while (false)

inline uint32_t GetDOPSampleRate(uint32_t dsd_speed) {
    switch (dsd_speed) {
        // 64x CD
    case 64:
        return 176400;
        // 128x CD
    case 128:
        return 352800;
        // 256x CD
    case 256:
        return 705600;
    default:
        throw NotSupportFormatException();
    }
}

class BassDSDLib final {
public:
    BassDSDLib() try
#ifdef XAMP_OS_WIN
        : module_(LoadModule("bassdsd.dll"))
#else
        : module_(LoadModule("libbassdsd.dylib"))
#endif
        , BASS_DSD_StreamCreateFile(module_, "BASS_DSD_StreamCreateFile") {
    }
    catch (const Exception & e) {
        XAMP_LOG_ERROR("{}", e.GetErrorMessage());
    }

    XAMP_DISABLE_COPY(BassDSDLib)

private:
    ModuleHandle module_;

public:
    DllFunction<HSTREAM(BOOL, void const *, QWORD, QWORD, DWORD, DWORD)> BASS_DSD_StreamCreateFile;
};

class BassFxLib final {
public:
    BassFxLib() try
#ifdef XAMP_OS_WIN
        : module_(LoadModule("bass_fx.dll"))
#else
        : module_(LoadModule("libbass_fx.dylib"))
#endif        
        , BASS_FX_TempoGetSource(module_, "BASS_FX_TempoGetSource")
        , BASS_FX_TempoCreate(module_, "BASS_FX_TempoCreate") {
    }
    catch (const Exception & e) {
        XAMP_LOG_ERROR("{}", e.GetErrorMessage());
    }

    XAMP_DISABLE_COPY(BassFxLib)

private:
    ModuleHandle module_;

public:    
    XAMP_DECLARE_DLL(BASS_FX_TempoGetSource) BASS_FX_TempoGetSource;
    XAMP_DECLARE_DLL(BASS_FX_TempoCreate) BASS_FX_TempoCreate;
};

struct BassPluginLoadTraits final {
    static HPLUGIN invalid() noexcept;

    static void close(HPLUGIN value);
};

class XAMP_STREAM_API BassLib final {
public:
    friend class Singleton<BassLib>;

    void Load();

    void Free();

    XAMP_ALWAYS_INLINE bool IsLoaded() const noexcept {
        return !plugins_.empty();
    }

    XAMP_DISABLE_COPY(BassLib)

    AlignPtr<BassFxLib> FxLib;
    AlignPtr<BassDSDLib> DSDLib;

private:
    BassLib() try
#ifdef XAMP_OS_WIN
        : module_(LoadModule("bass.dll"))
#else
        : module_(LoadModule("libbass.dylib"))
#endif
        , BASS_Init(module_, "BASS_Init")
        , BASS_SetConfig(module_, "BASS_SetConfig")
        , BASS_PluginLoad(module_, "BASS_PluginLoad")
        , BASS_PluginGetInfo(module_, "BASS_PluginGetInfo")
        , BASS_Free(module_, "BASS_Free")
        , BASS_StreamCreateFile(module_, "BASS_StreamCreateFile")
        , BASS_ChannelGetInfo(module_, "BASS_ChannelGetInfo")
        , BASS_StreamFree(module_, "BASS_StreamFree")
        , BASS_PluginFree(module_, "BASS_PluginFree")
        , BASS_ChannelGetData(module_, "BASS_ChannelGetData")
        , BASS_ChannelGetLength(module_, "BASS_ChannelGetLength")
        , BASS_ChannelBytes2Seconds(module_, "BASS_ChannelBytes2Seconds")
        , BASS_ChannelSeconds2Bytes(module_, "BASS_ChannelSeconds2Bytes")
        , BASS_ChannelSetPosition(module_, "BASS_ChannelSetPosition")
        , BASS_ErrorGetCode(module_, "BASS_ErrorGetCode")
        , BASS_ChannelGetAttribute(module_, "BASS_ChannelGetAttribute")
        , BASS_ChannelSetAttribute(module_, "BASS_ChannelSetAttribute")
        , BASS_StreamCreate(module_, "BASS_StreamCreate")
        , BASS_StreamPutData(module_, "BASS_StreamPutData")
        , BASS_ChannelSetFX(module_, "BASS_ChannelSetFX")
        , BASS_ChannelRemoveFX(module_, "BASS_ChannelRemoveFX")
        , BASS_FXSetParameters(module_, "BASS_FXSetParameters")
        , BASS_FXGetParameters(module_, "BASS_FXGetParameters") {
    }
    catch (const Exception &e) {
        XAMP_LOG_ERROR("{}", e.GetErrorMessage());
    }

    using BassPluginHandle = UniqueHandle<HPLUGIN, BassPluginLoadTraits>;

    HashMap<std::string, BassPluginHandle> plugins_;
    ModuleHandle module_;

public:
    XAMP_DECLARE_DLL(BASS_Init) BASS_Init;
    XAMP_DECLARE_DLL(BASS_SetConfig) BASS_SetConfig;
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
    XAMP_DECLARE_DLL(BASS_ErrorGetCode) BASS_ErrorGetCode;
    XAMP_DECLARE_DLL(BASS_ChannelGetAttribute) BASS_ChannelGetAttribute;
    XAMP_DECLARE_DLL(BASS_ChannelSetAttribute) BASS_ChannelSetAttribute;
    XAMP_DECLARE_DLL(BASS_StreamCreate) BASS_StreamCreate;
    XAMP_DECLARE_DLL(BASS_StreamPutData) BASS_StreamPutData;    
    XAMP_DECLARE_DLL(BASS_ChannelSetFX) BASS_ChannelSetFX;
    XAMP_DECLARE_DLL(BASS_ChannelRemoveFX) BASS_ChannelRemoveFX;
    XAMP_DECLARE_DLL(BASS_FXSetParameters) BASS_FXSetParameters;
    XAMP_DECLARE_DLL(BASS_FXGetParameters) BASS_FXGetParameters;

private:
    void LoadPlugin(std::string const & file_name);
};

struct XAMP_STREAM_API BassStreamTraits final {
    static HSTREAM invalid() noexcept;

    static void close(HSTREAM value);
};

using BassStreamHandle = UniqueHandle<HSTREAM, BassStreamTraits>;

}

