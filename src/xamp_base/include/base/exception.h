//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <exception>
#include <ostream>
#include <string>

#include <base/base.h>
#include <base/audioformat.h>

namespace xamp::base {

enum class Errors {
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
    XAMP_ERROR_NOT_SUPPORT_VARIABLE_RESAMPLE,
    XAMP_ERROR_NOT_FOUND_DLL_EXPORT_FUNC,
    _MAX_XAMP_ERROR_,
};

XAMP_BASE_API std::ostream& operator<<(std::ostream& ostr, Errors error);

class XAMP_BASE_API Exception : public std::exception {
public:
    explicit Exception(Errors error = Errors::XAMP_ERROR_SUCCESS, const std::string& message = "", std::string_view what = "");

    ~Exception() override = default;

	static std::string GetPlatformErrorMessage(int32_t err);

    const char * what() const noexcept override;

    virtual Errors GetError() const;

    const char* GetErrorMessage() const;

    virtual const char * GetExpression() const;

private:
    Errors error_;

protected:	
	std::string_view what_;
    std::string message_;
};

class XAMP_BASE_API PlatformSpecException : public Exception {
public:
    explicit PlatformSpecException(int32_t err);

    explicit PlatformSpecException(std::string_view what, int32_t err);

    ~PlatformSpecException() override = default;
};

#define XAMP_DECLARE_EXCEPTION_CLASS(ExceptionClassName) \
class XAMP_BASE_API ExceptionClassName final : public Exception {\
public:\
    ExceptionClassName();\
    ~ExceptionClassName() override = default;\
};

class XAMP_BASE_API DeviceUnSupportedFormatException final : public Exception{
public:
    explicit DeviceUnSupportedFormatException(const AudioFormat & format);

    ~DeviceUnSupportedFormatException() override = default;

private:
    AudioFormat format_;
};

class XAMP_BASE_API LoadDllFailureException final : public Exception {
public:
    explicit LoadDllFailureException(const std::string_view& dll_name);

    ~LoadDllFailureException() override = default;

private:
    std::string_view dll_name_;
};

class XAMP_BASE_API NotSupportVariableSampleRateException final : public Exception{
public:
    explicit NotSupportVariableSampleRateException(int32_t input_samplerate, int32_t output_samplerate);

    ~NotSupportVariableSampleRateException() override = default;
};

XAMP_DECLARE_EXCEPTION_CLASS(LibrarySpecErrorException)
XAMP_DECLARE_EXCEPTION_CLASS(DeviceNotInititalzedException)
XAMP_DECLARE_EXCEPTION_CLASS(DeviceInUseException)
XAMP_DECLARE_EXCEPTION_CLASS(DeviceNotFoundException)
XAMP_DECLARE_EXCEPTION_CLASS(FileNotFoundException)
XAMP_DECLARE_EXCEPTION_CLASS(NotSupportSampleRateException)
XAMP_DECLARE_EXCEPTION_CLASS(NotSupportFormatException)
XAMP_DECLARE_EXCEPTION_CLASS(StopStreamTimeoutException)
XAMP_DECLARE_EXCEPTION_CLASS(SampleRateChangedException)
XAMP_DECLARE_EXCEPTION_CLASS(ResamplerNotSupportSampleRateException)
XAMP_DECLARE_EXCEPTION_CLASS(NotFoundDllExportFuncException)

}
