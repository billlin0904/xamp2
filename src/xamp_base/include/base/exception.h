//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <exception>
#include <ostream>
#include <string>

#include <base/base.h>
#include <base/enum.h>
#include <base/audioformat.h>

namespace xamp::base {

MAKE_ENUM(Errors,
          XAMP_ERROR_SUCCESS = 0,
          XAMP_ERROR_PLATFORM_SPEC_ERROR,
          XAMP_ERROR_LIBRARY_SPEC_ERROR,
          XAMP_ERROR_DEVICE_NOT_INITIALIZED,
          XAMP_ERROR_DEVICE_UNSUPPORTED_FORMAT,
          XAMP_ERROR_DEVICE_IN_USE,
          XAMP_ERROR_DEVICE_NOT_FOUND,
          XAMP_ERROR_FILE_NOT_FOUND,
          XAMP_ERROR_NOT_SUPPORT_SAMPLERATE,
          XAMP_ERROR_NOT_SUPPORT_FORMAT,
          XAMP_ERROR_LOAD_DLL_FAILURE,
          XAMP_ERROR_STOP_STREAM_TIMEOUT,
          XAMP_ERROR_SAMPLERATE_CHANGED,
          XAMP_ERROR_NOT_SUPPORT_RESAMPLE_SAMPLERATE,
          XAMP_ERROR_NOT_FOUND_DLL_EXPORT_FUNC)

XAMP_BASE_API std::string GetPlatformErrorMessage(int32_t err);

XAMP_BASE_API std::string GetLastErrorMessage();

class XAMP_BASE_API Exception : public std::exception {
public:
    explicit Exception(Errors error = Errors::XAMP_ERROR_SUCCESS,
                       std::string const & message = "",
                       std::string_view what = "");
    
    virtual ~Exception() noexcept override = default;

    [[nodiscard]] char const * what() const noexcept override;

    [[nodiscard]] virtual Errors GetError() const noexcept;

    [[nodiscard]] char const * GetErrorMessage() const noexcept;

    [[nodiscard]] virtual const char * GetExpression() const noexcept;

    [[nodiscard]] char const* GetStackTrace() const noexcept;

    static std::string_view ErrorToString(Errors error);
private:
    Errors error_;

protected:	
	std::string what_;
    std::string message_;
    std::string stacktrace_;
};

class XAMP_BASE_API LibrarySpecException : public Exception {
public:
    LibrarySpecException(std::string const& message, std::string_view what = "");
};

class XAMP_BASE_API PlatformSpecException : public Exception {
public:
    PlatformSpecException();

    explicit PlatformSpecException(int32_t err);

    explicit PlatformSpecException(std::string_view what);

    PlatformSpecException(std::string_view what, int32_t err);

    virtual ~PlatformSpecException() override = default;
};

#define XAMP_DECLARE_EXCEPTION_CLASS(ExceptionClassName) \
class XAMP_BASE_API ExceptionClassName final : public Exception {\
public:\
    ExceptionClassName();\
    virtual ~ExceptionClassName() = default;\
};

class XAMP_BASE_API DeviceUnSupportedFormatException final : public Exception {
public:
    explicit DeviceUnSupportedFormatException(AudioFormat const & format);

    virtual ~DeviceUnSupportedFormatException() override = default;

private:
    AudioFormat format_;
};

class XAMP_BASE_API LoadDllFailureException final : public Exception {
public:
    explicit LoadDllFailureException(std::string_view dll_name);

    virtual ~LoadDllFailureException() override = default;

private:
    std::string_view dll_name_;
};

XAMP_DECLARE_EXCEPTION_CLASS(DeviceNotInititalzedException)
XAMP_DECLARE_EXCEPTION_CLASS(DeviceInUseException)
XAMP_DECLARE_EXCEPTION_CLASS(DeviceNotFoundException)
XAMP_DECLARE_EXCEPTION_CLASS(FileNotFoundException)
XAMP_DECLARE_EXCEPTION_CLASS(NotSupportSampleRateException)
XAMP_DECLARE_EXCEPTION_CLASS(NotSupportFormatException)
XAMP_DECLARE_EXCEPTION_CLASS(StopStreamTimeoutException)
XAMP_DECLARE_EXCEPTION_CLASS(SampleRateChangedException)
XAMP_DECLARE_EXCEPTION_CLASS(NotFoundDllExportFuncException)
XAMP_DECLARE_EXCEPTION_CLASS(NotSupportResampleSampleRateException)


#define BufferOverFlowThrow(expr) \
    do {\
        if (!(expr)) {\
        throw LibrarySpecException("Buffer overflow.", "Buffer overflow.");\
        }\
    } while(false)

}
