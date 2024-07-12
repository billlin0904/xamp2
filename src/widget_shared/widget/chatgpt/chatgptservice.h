//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#if 1
#include <QtConcurrent/QtConcurrent>
#include <QObject>
#include <QFuture>

#include <string>
#include <vector>
#include <widget/widget_shared.h>
#include <widget/baseservice.h>

class ChatGptInterop {
public:
    ChatGptInterop();

    XAMP_PIMPL(ChatGptInterop)

    bool initial();

    std::string getResponse(const std::string& prompt);

    std::vector<std::string> getResponses(const std::vector<std::string>& prompts);
private:
	class ChatGptInteropImpl;
    AlignPtr<ChatGptInteropImpl> impl_;
};

class XAMP_WIDGET_SHARED_EXPORT ChatGptService : public BaseService {
public:
	explicit ChatGptService(QObject* parent = nullptr);

    QFuture<bool> initialAsync();

    QFuture<std::string> getResponseAsync(const std::string& prompt);

    QFuture<std::vector<std::string>> getResponsesAsync(const std::vector<std::string>& prompts);

    QFuture<bool> cleanupAsync();
private:	
    ChatGptInterop* interop();

    LocalStorage<ChatGptInterop> interop_;
};

#endif
