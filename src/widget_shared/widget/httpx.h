//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QCoroTask>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QNetworkCookie>

#include <widget/util/str_util.h>
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

        QCoro::Task<QByteArray> postMultipart(const QString& file_path,
            const QString& field_name = "file"_str,
            const QString& mime_type = "application/octet-stream"_str);

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
        enum class BodyKind {
            NoBody,
            FormUrlEncoded,
            Json,
            Multipart
        };

        void setHeaders(QNetworkRequest& request, BodyKind body_kind);
        QString processReply(QNetworkReply* reply);
        QByteArray processBinaryReply(QNetworkReply* reply);
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
        QList<QNetworkCookie> cookies_;
    };
}
