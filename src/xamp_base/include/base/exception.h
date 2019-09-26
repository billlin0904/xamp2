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

enum Errors {
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
};

std::ostream& operator<<(std::ostream& ostr, Errors error);

class XAMP_BASE_API Exception : public std::exception {
public:
	explicit Exception(Errors error = XAMP_ERROR_UNKNOWN, const std::string& message = "");

	virtual ~Exception() = default;

	const char * what() const override;

	virtual Errors GetError() const;

	const char * GetErrorMessage() const;

	virtual const char * GetExpression() const;

protected:	
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

DECLARE_EXCEPTION_CLASS(LibrarySpecErrorException, Errors::XAMP_ERROR_LIBRARY_SPEC_ERROR)
DECLARE_EXCEPTION_CLASS(DeviceNotInititalzedException, Errors::XAMP_ERROR_DEVICE_NOT_INITIALIZED)
DECLARE_EXCEPTION_CLASS(DeviceInUseException, Errors::XAMP_ERROR_DEVICE_IN_USE)
DECLARE_EXCEPTION_CLASS(DeviceUnSupportedFormatException, Errors::XAMP_ERROR_DEVICE_UNSUPPORTED_FORMAT)
DECLARE_EXCEPTION_CLASS(DeviceNotFoundException, Errors::XAMP_ERROR_DEVICE_NOT_FOUND)
DECLARE_EXCEPTION_CLASS(FileNotFoundException, Errors::XAMP_ERROR_FILE_NOT_FOUND)
DECLARE_EXCEPTION_CLASS(NotSupportSampleRateException, Errors::XAMP_ERROR_NOT_SUPPORT_SAMPLERATE)
DECLARE_EXCEPTION_CLASS(NotSupportFormatException, Errors::XAMP_ERROR_NOT_SUPPORT_FORMAT)
DECLARE_EXCEPTION_CLASS(LoadDllFailureException, Errors::XAMP_ERROR_LOAD_DLL_FAILURE)

}
