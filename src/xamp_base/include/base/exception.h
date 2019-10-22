//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <exception>
#include <ostream>

#include <base/base.h>
#include <string>

namespace xamp::base {

enum class Errors {
    XAMP_ERROR_UNKNOWN,
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
    _MAX_XAMP_ERROR_,
};

XAMP_BASE_API std::ostream& operator<<(std::ostream& ostr, Errors error);

class XAMP_BASE_API Exception : public std::exception {
public:
    explicit Exception(Errors error = Errors::XAMP_ERROR_UNKNOWN, const std::string& message = "", const char* what = "");

    ~Exception() override = default;

    const char * what() const noexcept override;

    virtual Errors GetError() const;

    const char * GetErrorMessage() const;

    virtual const char * GetExpression() const;

private:
    Errors error_;

protected:	
    const char* what_;
    std::string message_;
};

#define XAMP_DECLARE_EXCEPTION_CLASS(ExceptionClassName) \
class XAMP_BASE_API ExceptionClassName : public Exception {\
public:\
    ExceptionClassName();\
    ~ExceptionClassName() override = default;\
};

XAMP_DECLARE_EXCEPTION_CLASS(LibrarySpecErrorException)
XAMP_DECLARE_EXCEPTION_CLASS(DeviceNotInititalzedException)
XAMP_DECLARE_EXCEPTION_CLASS(DeviceInUseException)
XAMP_DECLARE_EXCEPTION_CLASS(DeviceUnSupportedFormatException)
XAMP_DECLARE_EXCEPTION_CLASS(DeviceNotFoundException)
XAMP_DECLARE_EXCEPTION_CLASS(FileNotFoundException)
XAMP_DECLARE_EXCEPTION_CLASS(NotSupportSampleRateException)
XAMP_DECLARE_EXCEPTION_CLASS(NotSupportFormatException)
XAMP_DECLARE_EXCEPTION_CLASS(LoadDllFailureException)

}
