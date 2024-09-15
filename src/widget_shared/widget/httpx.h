//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QCoroTask>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QStringConverter>
#include <memory>

#include <base/object_pool.h>
#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>

class QNetworkAccessManager;
class QNetworkRequest;
class QNetworkReply;

namespace http {

    class XAMP_WIDGET_SHARED_EXPORT HttpClient {
    public:
        HttpClient(QNetworkAccessManager* nam, const QString& url, QObject* parent = nullptr);
        explicit HttpClient(const QString& url, QObject* parent = nullptr);
        ~HttpClient();

        void setUrl(const QString& url);
        void setTimeout(int timeout);

        HttpClient& setJson(const QString& json);
        HttpClient& param(const QString& name, const QVariant& value);
        HttpClient& params(const QMultiMap<QString, QVariant>& ps);
        HttpClient& addAccpetJsonHeader();

        QCoro::Task<QString> get();
        QCoro::Task<QString> post();
        QCoro::Task<QString> put(const QByteArray& data);
        QCoro::Task<QString> del();

		QCoro::Task<QByteArray> download();

        void setParams(const QUrlQuery& params);
        void setUserAgent(const QString& userAgent);
        void setHeader(const QString& name, const QString& value);

    private:
        void setHeaders(QNetworkRequest& request);
        QString processReply(QNetworkReply* reply);
        QString processEncoding(QNetworkReply* reply, const QByteArray &content);

        QNetworkAccessManager* manager_;
        QString url_;
        QUrlQuery params_;
        QString json_;
        bool use_json_{ false };
        int timeout_{ 120000 };  // Àq»{¶W®É 120 ¬í
        QString user_agent_;
        QHash<QString, QString> headers_;
        LoggerPtr logger_;
        QStringConverter::Encoding charset_ = QStringConverter::Encoding::Utf8;
        std::shared_ptr<ObjectPool<QByteArray>> buffer_pool_;
    };
}