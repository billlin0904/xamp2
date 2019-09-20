#include <base/exception.h>

namespace xamp::base {

Exception::Exception(Errors error, const std::string& message)
	: error_(error)
	, message_(message) {
}

char const* Exception::what() const {
	return what_.c_str();
}

Errors Exception::GetError() const {
	return error_;
}

const char * Exception::GetErrorMessage() const {
	return message_.c_str();
}

const char* Exception::GetExpression() const {
	return "";
}

}
