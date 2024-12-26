//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QCoroTask>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QNetworkCookie>
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

        void setUrl(const QString& url);
        void setTimeout(int timeout);

        HttpClient& setJson(const QString& json);
        HttpClient& param(const QString& name, const QVariant& value);
        HttpClient& params(const QMultiMap<QString, QVariant>& ps);
        HttpClient& addAcceptJsonHeader();

        QCoro::Task<QString> get();
        QCoro::Task<QString> post();
        QCoro::Task<QString> put(const QByteArray& data);
        QCoro::Task<QString> del();

		QCoro::Task<QByteArray> download();

        void setParams(const QUrlQuery& params);
        void setUserAgent(const QString& user_agent);
        void setHeader(const QString& name, const QString& value);

        HttpClient& addCookie(const QNetworkCookie& cookie) {
            cookies_.append(cookie);
            return *this;
        }

        HttpClient& addCookies(const QList<QNetworkCookie>& cookies) {
            cookies_.append(cookies);
            return *this;
        }
    private:
        void setHeaders(QNetworkRequest& request);
        QString processReply(QNetworkReply* reply);
        QString processEncoding(QNetworkReply* reply, const QByteArray &content);

        bool use_json_{ false };
        int timeout_{ 120000 };
        QNetworkAccessManager* manager_;
        QString url_;
        QUrlQuery params_;
        QString json_;
        QString user_agent_;
        QHash<QString, QString> headers_;
        LoggerPtr logger_;
        QStringConverter::Encoding charset_ = QStringConverter::Encoding::Utf8;
        std::shared_ptr<ObjectPool<QByteArray>> buffer_pool_;
        QList<QNetworkCookie> cookies_;
    };
}