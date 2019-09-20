//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <exception>

#include <base/base.h>
#include <string>

namespace xamp::base {

enum Errors {
	ERROR_UNKNOWN,
	ERROR_PLATFORM_SPEC_ERROR,
	ERROR_LIBRARY_SPEC_ERROR,
	ERROR_DEVICE_NOT_INITIALIZED,
	ERROR_DEVICE_UNSUPPORTED_FORMAT,
	ERROR_DEVICE_IN_USE,
	ERROR_DEVICE_NOT_FOUND,
	ERROR_FILE_NOT_FOUND,
	ERROR_NOT_SUPPORT_SAMPLERATE,
	ERROR_NOT_SUPPORT_FORMAT
};

class XAMP_BASE_API Exception : public std::exception {
public:
	virtual ~Exception() = default;

	const char * what() const override;

	virtual Errors GetError() const;

	const char * GetErrorMessage() const;

	virtual const char * GetExpression() const;

protected:
	explicit Exception(Errors error = Errors::ERROR_UNKNOWN, const std::string& message = "");

	std::string what_;
	std::string message_;

private:
	Errors error_;
};

#define DECLARE_EXCEPTION_CLASS(ExceptionClassName, error) \
class XAMP_BASE_API ExceptionClassName : public Exception {\
public:\
    ExceptionClassName()\
        : Exception(error) {\
    }\
};

DECLARE_EXCEPTION_CLASS(LibrarySpecErrorException, Errors::ERROR_LIBRARY_SPEC_ERROR)
DECLARE_EXCEPTION_CLASS(DeviceNotInititalzedException, Errors::ERROR_DEVICE_NOT_INITIALIZED)
DECLARE_EXCEPTION_CLASS(DeviceInUseException, Errors::ERROR_DEVICE_IN_USE)
DECLARE_EXCEPTION_CLASS(DeviceUnSupportedFormatException, Errors::ERROR_DEVICE_UNSUPPORTED_FORMAT)
DECLARE_EXCEPTION_CLASS(DeviceNotFoundException, Errors::ERROR_DEVICE_NOT_FOUND)
DECLARE_EXCEPTION_CLASS(FileNotFoundException, Errors::ERROR_FILE_NOT_FOUND)
DECLARE_EXCEPTION_CLASS(NotSupportSampleRateException, Errors::ERROR_NOT_SUPPORT_SAMPLERATE)
DECLARE_EXCEPTION_CLASS(NotSupportFormatException, Errors::ERROR_NOT_SUPPORT_FORMAT)

}
