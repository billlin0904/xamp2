//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/enum.h>
#include <base/audioformat.h>
#include <base/str_utilts.h>

#include <exception>
#include <ostream>
#include <string>

XAMP_BASE_NAMESPACE_BEGIN

/*
* Error code enum
* 
* <remarks>
* XAMP_ERROR_SUCCESS: Success
* XAMP_ERROR_PLATFORM_SPEC_ERROR: Platform specific error
* XAMP_ERROR_LIBRARY_SPEC_ERROR: Library specific error
* XAMP_ERROR_DEVICE_CREATE_FAILURE: Device create failure
* XAMP_ERROR_DEVICE_UNSUPPORTED_FORMAT: Device unsupported format
* XAMP_ERROR_DEVICE_IN_USE: Device in use
* XAMP_ERROR_DEVICE_NOT_FOUND: Device not found
* XAMP_ERROR_FILE_NOT_FOUND: File not found
* XAMP_ERROR_NOT_SUPPORT_SAMPLE_RATE: Not support sample rate
* XAMP_ERROR_NOT_SUPPORT_FORMAT: Not support format
* XAMP_ERROR_LOAD_DLL_FAILURE: Load dll failure
* XAMP_ERROR_STOP_STREAM_TIMEOUT: Stop stream timeout
* XAMP_ERROR_SAMPLE_RATE_CHANGED: Sample rate changed
* XAMP_ERROR_NOT_SUPPORT_RESAMPLE_SAMPLE_RATE: Not support resample sample rate
* XAMP_ERROR_NOT_FOUND_DLL_EXPORT_FUNC: Not found dll export function
* XAMP_ERROR_NOT_SUPPORT_EXCLUSIVE_MODE: Not support exclusive mode
* XAMP_ERROR_NOT_BUFFER_OVERFLOW: Not buffer overflow
* XAMP_ERROR_UNKNOWN: Unknown error
* </remarks>
*/
XAMP_MAKE_ENUM(Errors,
    XAMP_ERROR_SUCCESS = 0,
    XAMP_ERROR_PLATFORM_SPEC_ERROR,
    XAMP_ERROR_LIBRARY_SPEC_ERROR,
    XAMP_ERROR_DEVICE_CREATE_FAILURE,
    XAMP_ERROR_DEVICE_UNSUPPORTED_FORMAT,
    XAMP_ERROR_DEVICE_IN_USE,
    XAMP_ERROR_DEVICE_NOT_FOUND,
    XAMP_ERROR_DEVICE_NEED_SET_MATCH_FORMAT,
    XAMP_ERROR_FILE_NOT_FOUND,
    XAMP_ERROR_NOT_SUPPORT_SAMPLE_RATE,
    XAMP_ERROR_NOT_SUPPORT_FORMAT,
    XAMP_ERROR_LOAD_DLL_FAILURE,
    XAMP_ERROR_STOP_STREAM_TIMEOUT,
    XAMP_ERROR_SAMPLE_RATE_CHANGED,
    XAMP_ERROR_NOT_SUPPORT_RESAMPLE_SAMPLE_RATE,
    XAMP_ERROR_NOT_FOUND_DLL_EXPORT_FUNC,
    XAMP_ERROR_NOT_SUPPORT_EXCLUSIVE_MODE,
    XAMP_ERROR_NOT_BUFFER_OVERFLOW,
    XAMP_ERROR_UNKNOWN)

 /*
 * Exception class
 *
 */
class XAMP_BASE_API Exception : public std::exception {
 public:
     /*
     * Constructor.
     *
     * @param message: Message.
     * @param what: What.
     */
     explicit Exception(std::string const& message, std::string_view what = "");

     /*
     * Constructor.
     *
     * @param error: Error code.
     * @param message: Message.
     * @param what: What.
     */
     explicit Exception(Errors error = Errors::XAMP_ERROR_SUCCESS,
         std::string const& message = "",
         std::string_view what = "");

     /*
     * Destructor.
     */
     virtual ~Exception() noexcept override = default;

     /*
     * what function.
     */
     [[nodiscard]] char const* what() const noexcept override;

     /*
     * Get error code.
     */
     [[nodiscard]] virtual Errors GetError() const noexcept;

     /*
     * Get error message.
     */
     [[nodiscard]] char const* GetErrorMessage() const noexcept;

     /*
     * Get expression.
     */
     [[nodiscard]] virtual const char* GetExpression() const noexcept;

     /*
     * Get stack trace.
     */
     [[nodiscard]] char const* GetStackTrace() const noexcept;

     /*
     * Get error code string.
     */
     static std::string_view ErrorToString(Errors error);
 private:
     Errors error_;

 protected:
     std::string what_;
     std::string message_;
     std::string stacktrace_;
};

XAMP_BASE_API std::string GetPlatformErrorMessage(int32_t err);

XAMP_BASE_API std::string GetLastErrorMessage();

class XAMP_BASE_API LibraryException : public Exception {
public:
	explicit LibraryException(std::string const& message, std::string_view what = "");

    virtual ~LibraryException() override = default;
};

class XAMP_BASE_API PlatformException : public Exception {
public:
    PlatformException();

    explicit PlatformException(int32_t err);

    explicit PlatformException(std::string_view what);

    PlatformException(std::string_view what, int32_t err);

    virtual ~PlatformException() override = default;
};

#define XAMP_DECLARE_EXCEPTION_CLASS(ExceptionClassName) \
class XAMP_BASE_API ExceptionClassName final : public Exception {\
public:\
	ExceptionClassName();\
	explicit ExceptionClassName(std::string const& message);\
    virtual ~ExceptionClassName() override = default;\
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

class XAMP_BASE_API NotFoundDllExportFuncException final : public Exception {
public:
    explicit NotFoundDllExportFuncException(std::string_view func_name);

    virtual ~NotFoundDllExportFuncException() override = default;

private:
    std::string_view func_name_;
};

class XAMP_BASE_API DeviceNotFoundException final : public Exception {
public:
    DeviceNotFoundException();

    explicit DeviceNotFoundException(std::string_view device_name);

    virtual ~DeviceNotFoundException() override = default;

private:
    std::string_view device_name_;
};

XAMP_DECLARE_EXCEPTION_CLASS(DeviceCreateFailureException)
XAMP_DECLARE_EXCEPTION_CLASS(DeviceInUseException)
XAMP_DECLARE_EXCEPTION_CLASS(DeviceNeedSetMatchFormatException)
XAMP_DECLARE_EXCEPTION_CLASS(FileNotFoundException)
XAMP_DECLARE_EXCEPTION_CLASS(NotSupportSampleRateException)
XAMP_DECLARE_EXCEPTION_CLASS(NotSupportFormatException)
XAMP_DECLARE_EXCEPTION_CLASS(StopStreamTimeoutException)
XAMP_DECLARE_EXCEPTION_CLASS(SampleRateChangedException)
XAMP_DECLARE_EXCEPTION_CLASS(NotSupportResampleSampleRateException)
XAMP_DECLARE_EXCEPTION_CLASS(NotSupportExclusiveModeException)
XAMP_DECLARE_EXCEPTION_CLASS(BufferOverflowException)

template <typename E, typename... Args>
void Throw(std::string_view s, Args &&...args) {
    throw E(String::Format(s, std::forward<Args>(args)...).c_str());
}

template <typename E, typename T = bool, typename... Args>
void ThrowIf(T &&value, std::string_view s, Args &&...args) {
    if (!value) {
        Throw<E>(s, std::forward<Args>(args)...);
    }
}

XAMP_BASE_NAMESPACE_END