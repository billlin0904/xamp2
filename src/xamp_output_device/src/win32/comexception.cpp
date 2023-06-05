#include <output_device/win32/comexception.h>

#ifdef XAMP_OS_WIN
#include <output_device/win32/wasapi.h>

#include <base/platfrom_handle.h>
#include <base/base.h>

#include <comutil.h>
#include <comdef.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

static std::string MakeErrorMessage(HRESULT hr) {
    std::ostringstream ostr;
    ostr << "Hr code: 0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << hr << " (" << GetPlatformErrorMessage(hr) << ")";
    return ostr.str();
}

static std::string MakeFileNameAndLine(const Path& file_path, int32_t line_number) {
    std::ostringstream ostr;
    ostr << file_path.filename() << ":" << std::dec << line_number;
    return ostr.str();
}

ComException::ComException(long hresult, std::string_view expr, const Path& file_path, int32_t line_number)
	: PlatformException(hresult)
	, hr_(hresult)
	, expr_(expr) {	
	message_ = MakeErrorMessage(hresult);
    file_name_and_line_ = MakeFileNameAndLine(file_path, line_number);
    what_ = message_;
}

long ComException::GetHResult() const {
	return hr_;
}

std::string ComException::GetFileNameAndLine() const {
    return file_name_and_line_;
}

const char* ComException::GetExpression() const noexcept {
	return expr_.data();
}

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif
