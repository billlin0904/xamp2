#include <widget/httpx.h>

#include <QCoroNetworkReply>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QMetaEnum>
#include <QHttpPart>

#include <widget/networkdiskcache.h>
#include <widget/util/str_util.h>
#include <widget/util/zib_util.h>

#include "version.h"

namespace http {
    namespace {
        XAMP_DECLARE_LOG_NAME(Http);
        constexpr qsizetype kHttpBufferSize = 4096;

#define qCompare(s1, s2) (QString::compare(s1, s2, Qt::CaseInsensitive) == 0)

        bool isCompressEncoding(const QNetworkReply * reply) {
            Q_FOREACH(const auto & header_pair, reply->rawHeaderPairs()) {
                if (qCompare(QString::fromUtf8(header_pair.first), "content-encoding"_str)) {
                    return qCompare(QString::fromUtf8(header_pair.second), "gzip"_str) 
                        || qCompare(QString::fromUtf8(header_pair.second), "deflate"_str);
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
                auto fromCache = reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool();
                if (fromCache) {
                    stream << " [From Cache]";
                }
                else {
                    stream << " [From Network]";
                }
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
            if (!logger->ShouldLog(logger->GetLevel())) {
                return;
            }
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
        QByteArray data;
        if (!use_json_) {
            data = params_.toString(QUrl::FullyEncoded).toUtf8();
        }
        else {
            data = json_.toUtf8();
            auto result = gzipCompress(data, CompressType::COMPRESS_GZIP);
            if (result && result.value().size() < data.size()) {
            //if (result) {
                data = result.value();
                request.setRawHeader("Content-Encoding", "gzip");
                request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(data.size()));
            }
        }
        auto* reply = co_await manager_->post(request, data);
        logHttpRequest(logger,
            requestVerb(QNetworkAccessManager::PostOperation, request),
            request, 
            reply);
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
        logHttpRequest(logger,
            requestVerb(QNetworkAccessManager::PutOperation, request),
            request, 
            reply);
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
        logHttpRequest(logger,
            requestVerb(QNetworkAccessManager::DeleteOperation, request),
            request,
            reply);
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
        logHttpRequest(logger,
            requestVerb(QNetworkAccessManager::GetOperation, request),
            request,
            reply);
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
        } else {
            request.setRawHeader("User-Agent", QString(kDefaultUserAgent).toUtf8());
        }

        request.setRawHeader("Accept-Encoding", "gzip, deflate");
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

		request.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
			QNetworkRequest::PreferCache);
    }

    QString HttpClient::processReply(QNetworkReply* reply) {
        QString data;
        if (reply->error() == QNetworkReply::NoError) {
            data = processEncoding(reply, reply->readAll());
            reply->deleteLater();
        }
        else {
            auto error_response = processEncoding(reply, reply->readAll());
            const auto error_string = reply->errorString();
            XAMP_LOG_D(logger_, error_string.toStdString());
            reply->deleteLater();
        }
        return data;
    }

    QString HttpClient::processEncoding(QNetworkReply* reply,
        const QByteArray& content) {
        QScopedPointer<QTextStream> in;
        in.reset(new QTextStream(content));
        QString result;

        if (isCompressEncoding(reply)) {            
            const auto data = gzipDecompress(content);
            if (data) {
                in.reset(new QTextStream(data.value()));
                result.reserve(data.value().size());
                XAMP_LOG_D(logger_, "Decompressing content.");
            }
            else {
                XAMP_LOG_D(logger_, "Failed to decompress content ({}).", 
                    data.error());
            }
        }
        else {
            const auto content_length_var =
                reply->header(QNetworkRequest::ContentLengthHeader);
            auto content_length = kHttpBufferSize;
            if (content_length_var.isValid()) {
                content_length = content_length_var.toLongLong();
            }
            result.reserve(content_length);
        }

        in->setEncoding(charset_);

        while (!in->atEnd()) {
            result.append(in->readLine());
        }

        return result;
    }

    QCoro::Task<QByteArray> HttpClient::postMultipart(const QString& file_path,
        const QString& field_name,
        const QString& mime_type) {
        QUrl full_url(url_);
        // 如果原本就有可能帶 query param，可以視需求：
        if (!params_.isEmpty()) {
            full_url.setQuery(params_);
            params_.clear();
        }

        QNetworkRequest request(full_url);
        // 設定你要的 headers, e.g. Accept, Cookie, etc. 
        // 注意：不要自己硬填 Content-Type = "multipart/form-data"，讓 QHttpMultiPart 自己帶。
        setHeaders(request);
        request.setRawHeader("Accept", "application/json");
       
        // 構建 multiPart
        auto* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

        auto file_name = QFileInfo(file_path).fileName();
        // 準備檔案
        QHttpPart filePart;
        filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
            QVariant(qFormat(R"(form-data; name="%1"; filename="%2")")
                .arg(field_name)
                .arg(file_name)));
        filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(mime_type));

        auto* file = new QFile(file_path);
        if (!file->open(QIODevice::ReadOnly)) {
            // 打不開檔案就直接丟錯或回傳失敗
            file->deleteLater();
            delete multiPart;
            // 也可改用 throw std::runtime_error(...) 或 co_return 空字串
            co_return QByteArray();
        }
        filePart.setBodyDevice(file);
        file->setParent(multiPart); // 生命週期交給 multiPart

        multiPart->append(filePart);

		request.setHeader(QNetworkRequest::ContentTypeHeader,
			QVariant(qFormat("multipart/form-data; boundary=%1"_str)
				.arg(QString::fromUtf8(multiPart->boundary()))));

        // 送出
        auto* reply = co_await manager_->post(request, multiPart);
        // 記得把 multiPart 的 parent 設成 reply，等 reply 結束後自動清理
        multiPart->setParent(reply);

        // 你原本的 log
        logHttpRequest(logger_, "POST"_str, request, reply);

        // 處理錯誤或讀取資料
        QByteArray response_data;
        if (reply->error() == QNetworkReply::NoError) {
            // 讀取回應原始資料
            response_data = reply->readAll();

            // 假如伺服器回傳 gzip 壓縮，可如原本那樣處理
            if (isCompressEncoding(reply)) {
                XAMP_LOG_D(logger_, "Decompressing gzip content.");
                auto result = gzipDecompress(response_data);
                if (result) {
                    response_data = result.value();
                }
            }
        }
        else {
            // 讀取錯誤訊息
            const auto error_string = reply->errorString();
            XAMP_LOG_E(logger_, "Http Post Multipart failed: {}", error_string.toStdString());
            // 這邊可考慮:
            // throw std::runtime_error(error_string.toStdString());
            // 或只是回傳空 QByteArray()
        }

        reply->deleteLater();
        co_return response_data;
    }

}
