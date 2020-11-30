//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QString>
#include <QVariant>
#include <QNetworkAccessManager>

namespace http {

enum class HttpMethod {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE
};

class HttpClient {
public:
    HttpClient(const QString &url, QNetworkAccessManager* manager = nullptr);

    ~HttpClient();

    HttpClient& param(const QString &name, const QVariant &value);

    HttpClient& params(const QMap<QString, QVariant> &ps);

    HttpClient& json(const QString &json);

    HttpClient& header(const QString &name, const QString &value);

    HttpClient& headers(const QMap<QString, QString> nameValues);

    HttpClient& success(std::function<void (const QString &)> successHandler);

    HttpClient& error(std::function<void(const QString&)> errorHandler);

    void download(std::function<void (const QByteArray &)> downloadHandler, std::function<void(const QString&)> errorHandler = nullptr);

    void downloadFile(const QString &file_name, std::function<void(const QString&)> downloadHandler, std::function<void(const QString&)> errorHandler = nullptr);

    void get();

    void post();

    void setUserAgent(const QString &user_agent);

private:
    class HttpClientImpl;
    QScopedPointer<HttpClientImpl> impl_;
};


}
