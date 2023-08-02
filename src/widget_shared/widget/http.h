//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QString>
#include <QVariant>
#include <QNetworkAccessManager>

#include <widget/widget_shared_global.h>

namespace http {

enum class HttpMethod {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE
};

class XAMP_WIDGET_SHARED_EXPORT HttpClient {
public:
    explicit HttpClient(const QUrl& url, QObject* parent = nullptr);
	
    explicit HttpClient(const QString &url, QObject* parent = nullptr);

    ~HttpClient();

    void setTimeout(int timeout);

    HttpClient& param(const QString &name, const QVariant &value);

    HttpClient& params(const QMap<QString, QVariant> &ps);

    HttpClient& json(const QString &json);

    HttpClient& header(const QString &name, const QString &value);

    HttpClient& headers(const QMap<QString, QString> nameValues);

    HttpClient& success(std::function<void (const QString &)> successHandler);

    HttpClient& error(std::function<void(const QString&)> errorHandler);

    HttpClient& progress(std::function<void(qint64, qint64)> progressHandler);

    void download(std::function<void (const QByteArray &)> downloadHandler, std::function<void(const QString&)> errorHandler = nullptr);

    void downloadFile(const QString &file_name, std::function<void(const QString&)> downloadHandler, std::function<void(const QString&)> errorHandler = nullptr);

    void get();

    void post();

    void setUserAgent(const QString &user_agent);

private:
    class HttpClientImpl;
    QSharedPointer<HttpClientImpl> impl_;
};

}
