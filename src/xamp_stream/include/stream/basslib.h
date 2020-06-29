//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstdint>

#include <bass/bass.h>
#include <bass/bassdsd.h>

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
inline constexpr int32_t kPcmSampleRate441 = { 44100 };

#define BassIfFailedThrow(result) \
    do {\
        if (!(result)) {\
            throw BassException(BassLib::Instance().BASS_ErrorGetCode());\
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

class XAMP_STREAM_API BassLib final {
public:
    static XAMP_ALWAYS_INLINE BassLib & Instance() {
        static BassLib lib;
        return lib;
    }

    ~BassLib() {
        plugins_.clear();

        if (module_.is_valid()) {
            try {
                BassLib::Instance().BASS_Free();
            }
            catch (const Exception & e) {
                XAMP_LOG_INFO("{}", e.what());
            }
        }
    }

    XAMP_ALWAYS_INLINE bool IsLoaded() const noexcept {
        return !plugins_.empty();
    }

    void Load() {
        if (IsLoaded()) {
            return;
        }

        BassLib::Instance().BASS_Init(0, 44100, 0, nullptr, nullptr);
#ifdef XAMP_OS_WIN
        // Disable Media Foundation
        BassLib::Instance().BASS_SetConfig(BASS_CONFIG_MF_DISABLE, true);
        BassLib::Instance().BASS_SetConfig(BASS_CONFIG_MF_VIDEO, false);
        LoadPlugin("bass_aac.dll");
        LoadPlugin("bassflac.dll");
        LoadPlugin("bass_ape.dll");
        LoadPlugin("bass_fx.dll");
#else
        LoadPlugin("bass_aac.dylib");
#endif
    }

    XAMP_DISABLE_COPY(BassLib)

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
        , BASS_ChannelSetFX(module_, "BASS_ChannelSetFX")
        , BASS_FXSetParameters(module_, "BASS_FXSetParameters")
        , BASS_FXGetParameters(module_, "BASS_FXGetParameters")
        , BASS_StreamCreate(module_, "BASS_StreamCreate")
        , BASS_StreamPutData(module_, "BASS_StreamPutData") {
    }
    catch (const Exception &e) {
        XAMP_LOG_ERROR("{}", e.GetErrorMessage());
    }

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
    XAMP_DECLARE_DLL(BASS_ChannelSetFX) BASS_ChannelSetFX;
    XAMP_DECLARE_DLL(BASS_FXSetParameters) BASS_FXSetParameters;
    XAMP_DECLARE_DLL(BASS_FXGetParameters) BASS_FXGetParameters;
    XAMP_DECLARE_DLL(BASS_StreamCreate) BASS_StreamCreate;
    XAMP_DECLARE_DLL(BASS_StreamPutData) BASS_StreamPutData;

    AlignPtr<BassDSDLib> DSDLib;

private:
    template <typename T>
    constexpr uint8_t HiByte(T val) noexcept {
        return static_cast<uint8_t>(val >> 8);
    }

    template <typename T>
    constexpr uint8_t LowByte(T val) noexcept {
        return static_cast<uint8_t>(val);
    }

    template <typename T>
    constexpr uint16_t HiWord(T val) noexcept {
        return static_cast<uint16_t>((uint32_t(val) >> 16) & 0xFFFF);
    }

    template <typename T>
    constexpr uint16_t LoWord(T val) noexcept {
        return static_cast<uint16_t>(uint32_t(val) & 0xFFFF);
    }

    void LoadPlugin(std::string const & file_name) {
        BassPluginHandle plugin(BassLib::Instance().BASS_PluginLoad(file_name.c_str(), 0));
        if (!plugin) {
            XAMP_LOG_DEBUG("Load {} failure.", file_name);
            return;
        }

        const auto info = BassLib::Instance().BASS_PluginGetInfo(plugin.get());
        const uint32_t major_version(HiByte(HiWord(info->version)));
        const uint32_t minor_version(LowByte(HiWord(info->version)));
        const uint32_t micro_version(HiByte(LoWord(info->version)));
        const uint32_t build_version(LowByte(LoWord(info->version)));

        std::ostringstream ostr;
        ostr << major_version << "."
             << minor_version << "."
             << micro_version << "."
             << build_version;
        XAMP_LOG_DEBUG("Load {} {} successfully.", file_name, ostr.str());

        plugins_[file_name] = std::move(plugin);
    }

    struct BassPluginLoadTraits final {
        static HPLUGIN invalid() noexcept {
            return 0;
        }

        static void close(HPLUGIN value) noexcept {
            BassLib::Instance().BASS_PluginFree(value);
        }
    };

    using BassPluginHandle = UniqueHandle<HPLUGIN, BassPluginLoadTraits>;

    RobinHoodHashMap<std::string, BassPluginHandle> plugins_;
};

struct XAMP_STREAM_API BassStreamTraits final {
    static HSTREAM invalid() noexcept {
        return 0;
    }

    static void close(HSTREAM value) noexcept {
        BassLib::Instance().BASS_StreamFree(value);
    }
};

using BassStreamHandle = UniqueHandle<HSTREAM, BassStreamTraits>;

}

