//=====================================================================================================================
// Copyright (c) 2018-2024 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QString>
#include <QVariant>
#include <QNetworkAccessManager>

#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>
#include <base/object_pool.h>

namespace http {

enum class HttpMethod {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_HEAD,
    HTTP_DELETE
};

class XAMP_WIDGET_SHARED_EXPORT HttpClient final {
public:
    static constexpr size_t kBufferPoolSize = 256;

    HttpClient(QNetworkAccessManager* nam, 
        std::shared_ptr<ObjectPool<QByteArray>> buffer_pool,
        const QString& url,
        QObject* parent = nullptr);

    explicit HttpClient(const QUrl& url, QObject* parent = nullptr);
	
    explicit HttpClient(const QString &url = QString(), QObject* parent = nullptr);

    ~HttpClient();

    void setTimeout(int timeout);

    void setUrl(const QString& url);

    void setUserAgent(const QString& user_agent);

    HttpClient& param(const QString &name, const QVariant &value);

    HttpClient& params(const QMap<QString, QVariant> &ps);

    HttpClient& json(const QString &json);

    HttpClient& header(const QString &name, const QString &value);

    HttpClient& headers(const QMap<QString, QString>& name_values);

    HttpClient& success(std::function<void(const QUrl&, const QString &)> success_handler);

    HttpClient& error(std::function<void(const QUrl&, const QString&)> error_handler);

    HttpClient& progress(std::function<void(qint64, qint64)> progress_handler);

    void download(std::function<void(const QByteArray &)> download_handler,
        std::function<void(const QUrl&, const QString&)> error_handler = nullptr);

    void downloadFile(const QString &file_name, 
        const std::function<void(const QString&)>& download_handler,
        std::function<void(const QUrl&, const QString&)> error_handler = nullptr);

    QNetworkReply* get();

    QNetworkReply* post();

    QNetworkReply* head();

    QNetworkReply* del();

private:    
    class HttpClientImpl;
    QSharedPointer<HttpClientImpl> impl_;
    std::shared_ptr<ObjectPool<QByteArray>> buffer_pool_;
};

}
