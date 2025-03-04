#include <widget/httpx.h>

#include <QCoroNetworkReply>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QMetaEnum>

#include <widget/networkdiskcache.h>
#include <widget/util/str_util.h>
#include <widget/util/zib_util.h>

namespace http {
    namespace {
        XAMP_DECLARE_LOG_NAME(Http);
        constexpr qsizetype kHttpBufferSize = 4096;

#define qCompare(s1, s2) (QString::compare(s1, s2, Qt::CaseInsensitive) == 0)

        bool isZipEncoding(const QNetworkReply * reply) {
            Q_FOREACH(const auto & header_pair, reply->rawHeaderPairs()) {
                if (qCompare(QString::fromUtf8(header_pair.first), "content-encoding"_str)
                    && qCompare(QString::fromUtf8(header_pair.second), "gzip"_str)) {
                    return true;
                }
            }
            return false;
        }

        ConstexprQString networkErrorToString(QNetworkReply::NetworkError code) {
            const auto* mo = &QNetworkReply::staticMetaObject;
            const int index = mo->indexOfEnumerator("NetworkError");
            if (index == -1)
                return kEmptyString;
            const auto qme = mo->enumerator(index);
            return { qme.valueToKey(code) };
        }

        void logHttpRequest(const LoggerPtr& logger,
            const ConstexprQString& verb,
            const QString& url,
            const QNetworkRequest& request,
            const QNetworkReply* reply) {
            auto content_length = 0U;

            QString msg;
            QTextStream stream(&msg);
            stream.setEncoding(QStringConverter::Utf8);

            if (!reply) {
                stream << "Request: ";
            }
            else {
                stream << "Response: ";
            }

            stream << verb;
            if (reply) {
                stream << " " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            }

            stream << " " << url << " Header: { ";

            QString content_type;
            if (!reply) {
                const auto header_list = request.rawHeaderList();
                Q_FOREACH(const auto & head, header_list) {
                    stream << head << ": ";
                    stream << request.rawHeader(head);
                    stream << ", ";
                }
                content_length = request.header(QNetworkRequest::ContentLengthHeader).toUInt();
                content_type = request.header(QNetworkRequest::ContentTypeHeader).toString();
            }
            else {
                auto header_list = reply->rawHeaderList();
                Q_FOREACH(const auto & head, header_list) {
                    stream << head << ": ";
                    stream << reply->rawHeader(head);
                    stream << ", ";
                }
                content_length = reply->header(QNetworkRequest::ContentLengthHeader).toUInt();
                content_type = reply->header(QNetworkRequest::ContentTypeHeader).toString();
            }
            stream << "} Data: [";
            if (content_length > 0) {
                stream << formatBytes(content_length) << " of " << content_type << " data";
            }
            stream << "]";
            XAMP_LOG_D(logger, "{}", msg.toStdString());
        }

        ConstexprQString requestVerb(QNetworkAccessManager::Operation operation, const QNetworkRequest& request) {
            switch (operation) {
            case QNetworkAccessManager::HeadOperation:
                return "HEAD"_str;
            case QNetworkAccessManager::GetOperation:
                return "GET"_str;
            case QNetworkAccessManager::PutOperation:
                return "PUT"_str;
            case QNetworkAccessManager::PostOperation:
                return "POST"_str;
            case QNetworkAccessManager::DeleteOperation:
                return "DELETE"_str;
            default:
                return "UNKNOWN"_str;
            }
            Q_UNREACHABLE();
        }

        void logHttpRequest(const LoggerPtr& logger,
            const ConstexprQString& verb,
            const QNetworkRequest& request,
            const QNetworkReply* reply = nullptr) {
            logHttpRequest(logger, verb, request.url().toString(), request, reply);
        }
    }

    HttpClient::HttpClient(QNetworkAccessManager* nam,
        const QString& url, QObject* parent)
        : manager_(nam)
		, url_(url) {
    	logger_ = XampLoggerFactory.GetLogger(kHttpLoggerName);
        manager_->setCache(new NetworkDiskCache(parent));
    }

    HttpClient::HttpClient(const QString& url, QObject* parent)
        : HttpClient(new QNetworkAccessManager(parent), url, parent) {
    }

    void HttpClient::setUrl(const QString& url) {
        url_ = url;
    }

    void HttpClient::setTimeout(int timeout) {
        timeout_ = timeout;
    }

    HttpClient& HttpClient::addAcceptJsonHeader() {
		headers_["Accept"_str] = "application/json"_str;
		return *this;
    }

    HttpClient& HttpClient::param(const QString& name, const QVariant& value) {
        params_.addQueryItem(name, value.toString());
        return *this;
    }

    HttpClient& HttpClient::params(const QMultiMap<QString, QVariant>& ps) {
        for (auto itr = ps.cbegin(); 
            itr != ps.cend(); ++itr) {
            params_.addQueryItem(itr.key(), itr.value().toString());
        }
        return *this;
    }

    QCoro::Task<QString> HttpClient::get() {
        QUrl full_url(url_);
        if (!params_.isEmpty()) {
            full_url.setQuery(params_);
            params_.clear();
        }
        QNetworkRequest request(full_url);
        setHeaders(request);
        auto logger = logger_;
        auto* reply = co_await manager_->get(request);
        logHttpRequest(logger, requestVerb(QNetworkAccessManager::GetOperation,
            request), request, reply);
        co_return processReply(reply);
    }

    QCoro::Task<QString> HttpClient::post() {
	    QUrl full_url(url_);
        QNetworkRequest request(full_url);
        setHeaders(request);
        auto logger = logger_;
		const auto data = use_json_ ? json_.toUtf8()
    	: params_.toString(QUrl::FullyEncoded).toUtf8();
        auto* reply = co_await manager_->post(request, data);
        logHttpRequest(logger, requestVerb(QNetworkAccessManager::PostOperation,
            request), request, reply);
        co_return processReply(reply);
    }

    QCoro::Task<QString> HttpClient::put(const QByteArray& data) {
        QUrl full_url(url_);
        if (!params_.isEmpty()) {
            full_url.setQuery(params_);
            params_.clear();
        }
        QNetworkRequest request(full_url);
        setHeaders(request);
        auto logger = logger_;
        auto* reply = co_await manager_->put(request, data);
        logHttpRequest(logger, requestVerb(QNetworkAccessManager::PutOperation, 
            request), request, reply);
        co_return processReply(reply);
    }

    QCoro::Task<QString> HttpClient::del() {
        QUrl full_url(url_);
        if (!params_.isEmpty()) {
            full_url.setQuery(params_);
            params_.clear();
        }
        QNetworkRequest request(full_url);
        setHeaders(request);
        auto logger = logger_;
        auto* reply = co_await manager_->deleteResource(request);
        logHttpRequest(logger, requestVerb(QNetworkAccessManager::DeleteOperation,
            request), request, reply);
        co_return processReply(reply);
    }

    QCoro::Task<QByteArray> HttpClient::download() {
        QUrl full_url(url_);
        if (!params_.isEmpty()) {
            full_url.setQuery(params_);
            params_.clear();
        }
        QNetworkRequest request(full_url);
        setHeaders(request);
        auto logger = logger_;
        auto* reply = co_await manager_->get(request);
        logHttpRequest(logger, requestVerb(QNetworkAccessManager::GetOperation,
            request), request, reply);
		co_return reply->readAll();
    }

    void HttpClient::setParams(const QUrlQuery& params) {
        params_ = params;
    }

    HttpClient& HttpClient::setJson(const QString& json) {
        json_ = json;
        use_json_ = true;
        addAcceptJsonHeader();
        return *this;
    }

    void HttpClient::setUserAgent(const QString& user_agent) {
        user_agent_ = user_agent;
    }

    void HttpClient::setHeader(const QString& name, const QString& value) {
        headers_[name] = value;
    }

    void HttpClient::setHeaders(QNetworkRequest& request) {
        if (use_json_) {
            request.setHeader(QNetworkRequest::ContentTypeHeader, 
                "application/json; charset=utf-8"_str);
        }
        else {
            request.setHeader(QNetworkRequest::ContentTypeHeader, 
                "application/x-www-form-urlencoded"_str);
        }

        for (auto i = headers_.cbegin(); 
            i != headers_.cend(); ++i) {
            request.setRawHeader(i.key().toUtf8(), i.value().toUtf8());
        }

        if (!user_agent_.isEmpty()) {
            request.setRawHeader("User-Agent", user_agent_.toUtf8());
        }

        request.setRawHeader("Accept-Encoding", "gzip");
        request.setRawHeader("Connection", "keep-alive");

        if (!cookies_.isEmpty()) {
            QString cookie_header;
            for (const auto& cookie : cookies_) {
                if (!cookie_header.isEmpty()) {
                    cookie_header += "; "_str;
                }
                cookie_header += QString::fromUtf8(cookie.name())
            	+ "="_str
            	+ QString::fromUtf8(cookie.value());
            }
            request.setRawHeader("Cookie", cookie_header.toUtf8());
        }
    }

    QString HttpClient::processReply(QNetworkReply* reply) {
        QString data;
        if (reply->error() == QNetworkReply::NoError) {
            data = processEncoding(reply, reply->readAll());
            reply->deleteLater();
        }
        else {
            const auto error_string = reply->errorString();
            reply->deleteLater();
        }
        return data;
    }

    QString HttpClient::processEncoding(QNetworkReply* reply,
        const QByteArray& content) {
        QScopedPointer<QTextStream> in;
        try
        {
            if (isZipEncoding(reply)) {
				XAMP_LOG_D(logger_, "Decompressing gzip content.");
                const auto data = gzipDecompress(content);
                in.reset(new QTextStream(data));
            }
            else {
                in.reset(new QTextStream(content));
            }
        }
        catch (...)
        {
            in.reset(new QTextStream(content));
        }

        const auto content_length_var = 
            reply->header(QNetworkRequest::ContentLengthHeader);
        auto content_length = kHttpBufferSize;
        if (content_length_var.isValid()) {
            content_length = content_length_var.toLongLong();
        }

        QString result;
        result.reserve(content_length);
        in->setEncoding(charset_);

        while (!in->atEnd()) {
            result.append(in->readLine());
        }

        return result;
    }

}
