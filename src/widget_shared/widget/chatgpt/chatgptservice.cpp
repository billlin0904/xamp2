#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <cstdlib>

#include <widget/chatgpt/chatgptservice.h>

namespace py = pybind11;

void dumpObject(LoggerPtr logger, const py::object& obj) {
    XAMP_LOG_D(logger, "{}", py::str(obj).cast<std::string>());
}

XAMP_DECLARE_LOG_NAME(ChatGptService);
XAMP_DECLARE_LOG_NAME(ChatGptInterop);

class ChatGptInterop::ChatGptInteropImpl {
public:    
    ChatGptInteropImpl();

    bool initial();

    std::string getResponse(const std::string& prompt);

    std::vector<std::string> getResponses(const std::vector<std::string>& prompts);
private:    
    std::string api_key_;
    py::module openai_;
    py::object chat_completion_;
    LoggerPtr logger_;
};

ChatGptInterop::ChatGptInteropImpl::ChatGptInteropImpl() {
    chat_completion_ = py::none();    
    logger_ = XampLoggerFactory.GetLogger(XAMP_LOG_NAME(ChatGptInterop));
}

bool ChatGptInterop::ChatGptInteropImpl::initial() {
    if (!chat_completion_.is_none()) {
        return false;
    }
	openai_ = py::module_::import("openai");
    chat_completion_ = openai_.attr("ChatCompletion");    
    return true;
}

std::string ChatGptInterop::ChatGptInteropImpl::getResponse(const std::string& prompt) {
    py::dict params;
    //params["model"] = "gpt-3.5-turbo-0125";
    params["model"] = "gpt-4o-2024-05-13";

    py::dict system_dict;
    system_dict["role"] = "system";
    system_dict["content"] = "You are a music analysis assistant.";

    py::dict user_dict;
    user_dict["role"] = "user";
    user_dict["content"] = prompt;

    params["messages"] = py::list(py::make_tuple(system_dict, user_dict));

    py::object OpenAI = openai_.attr("OpenAI");
    py::object client = OpenAI();

    py::object response = client.attr("chat").attr("completions").attr("create")(**params);
    dumpObject(logger_, response);

    py::list choices = response.attr("choices");
    if (choices.size() == 0) {
        return "";
    }

    py::dict first_choice = choices[0].cast<py::dict>();
    py::dict message = first_choice["message"].cast<py::dict>();
    std::string content = message["content"].cast<std::string>();

    return content;
}

std::vector<std::string> ChatGptInterop::ChatGptInteropImpl::getResponses(const std::vector<std::string>& prompts) {
    std::vector<std::string> responses;
    for (const auto& prompt : prompts) {
        responses.push_back(getResponse(prompt));
    }
    return responses;
}

ChatGptInterop::ChatGptInterop()
    : impl_(MakeAlign<ChatGptInteropImpl>()) {
}

XAMP_PIMPL_IMPL(ChatGptInterop)

bool ChatGptInterop::initial() {
	return impl_->initial();
}

std::string ChatGptInterop::getResponse(const std::string& prompt) {
	return impl_->getResponse(prompt);
}

std::vector<std::string> ChatGptInterop::getResponses(const std::vector<std::string>& prompts) {
	return impl_->getResponses(prompts);
}

ChatGptService::ChatGptService(QObject* parent)
    : BaseService(parent) {
    logger_ = XampLoggerFactory.GetLogger(XAMP_LOG_NAME(ChatGptService));
}

ChatGptInterop* ChatGptService::interop() {
    return interop_.get();
}

QFuture<bool> ChatGptService::initialAsync() {
    return invokeAsync([this]() {
        py::gil_scoped_acquire guard{};
        return interop()->initial();
        });
}

QFuture<std::string> ChatGptService::getResponseAsync(const std::string& prompt) {
    return invokeAsync([this, prompt]() {
        py::gil_scoped_acquire guard{};
        return interop()->getResponse(prompt);
        });
}

QFuture<std::vector<std::string>> ChatGptService::getResponsesAsync(const std::vector<std::string>& prompts) {
    return invokeAsync([this, prompts]() {
        py::gil_scoped_acquire guard{};
        return interop()->getResponses(prompts);
        });
}

QFuture<bool> ChatGptService::cleanupAsync() {
    return invokeAsync([this]() {
        py::gil_scoped_acquire guard{};
        interop_.reset();
        return true;
        }, InvokeType::INVOKE_IMMEDIATELY);
}
