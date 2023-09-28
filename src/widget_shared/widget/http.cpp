#include <widget/http.h>

#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QUrl>
#include <QTemporaryFile>
#include <QNetworkProxy>
#include <QThread>
#include <QMetaEnum>

#include <memory>

#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/dll.h>

#include <version.h>
#include <widget/str_utilts.h>
#include <widget/zib_utiltis.h>
#include <widget/widget_shared.h>


namespace http {

XAMP_DECLARE_LOG_NAME(Http);

namespace {
    constexpr int32_t kHttpDefaultTimeout = 3000;
    constexpr size_t kHttpBufferSize = 1024;

    bool IsZipEncoding(QNetworkReply const* reply) {
        Q_FOREACH(const auto & header_pair, reply->rawHeaderPairs()) {
            if ((header_pair.first == "Content-Encoding") && (header_pair.second == "gzip")) {
                return true;
            }
        }
        return false;
    }

    ConstLatin1String NetworkErrorToString(QNetworkReply::NetworkError code) {
        const auto* mo = &QNetworkReply::staticMetaObject;
        const int index = mo->indexOfEnumerator("NetworkError");
        if (index == -1)
            return kEmptyString;
        const auto qme = mo->enumerator(index);
        return { qme.valueToKey(code) };
    }

    void LogHttpRequest(const LoggerPtr& logger,
        const ConstLatin1String& verb,
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
            stream << FormatBytes(content_length) << " of " << content_type << " data";
        }
        stream << "]";
        XAMP_LOG_D(logger, msg.toStdString());
    }

    ConstLatin1String RequestVerb(QNetworkAccessManager::Operation operation, const QNetworkRequest& request) {
        switch (operation) {
        case QNetworkAccessManager::HeadOperation:
            return qTEXT("HEAD");
        case QNetworkAccessManager::GetOperation:
            return qTEXT("GET");
        case QNetworkAccessManager::PutOperation:
            return qTEXT("PUT");
        case QNetworkAccessManager::PostOperation:
            return qTEXT("POST");
        case QNetworkAccessManager::DeleteOperation:
            return qTEXT("DELETE");
        case QNetworkAccessManager::CustomOperation:
            return qTEXT(request.attribute(QNetworkRequest::CustomVerbAttribute).toByteArray());
        case QNetworkAccessManager::UnknownOperation:
            break;
        }
        Q_UNREACHABLE();
    }

    void LogHttpRequest(const LoggerPtr& logger,
        const ConstLatin1String& verb,
        const QNetworkRequest& request,
        const QNetworkReply* reply = nullptr) {
        LogHttpRequest(logger, verb, request.url().toString(), request, reply);
    }
}

struct HttpContext {
    bool use_internal{false};
    QString charset{ kDefaultCharset };
    QString user_agent{ kDefaultUserAgent };
    QNetworkAccessManager* manager{nullptr};
    std::function<void (const QString &)> success_handler;
    std::function<void(const QString&)> error_handler;
    std::function<void(qint64, qint64)> progress_handler;
    LoggerPtr logger;
};

class XAMP_WIDGET_SHARED_EXPORT HttpClient::HttpClientImpl {
public:
	HttpClientImpl(const QString &url, QObject* parent = nullptr);

    ~HttpClientImpl();

    HttpContext CreateHttpContext() const;

    void SetTimeout(int timeout);

    static QNetworkRequest CreateHttpRequest(QSharedPointer<HttpClientImpl> d, HttpMethod method);

    static void ExecuteQuery(QSharedPointer<HttpClientImpl> d, HttpMethod method);

    static void download(QSharedPointer<HttpClientImpl> d, std::function<void (const QByteArray &)> ready_read);

    static QString ReadReply(QNetworkReply *reply, const QString &charset);

    static void HandleFinish(const HttpContext& context, QNetworkReply *reply, const QString &success_message);

    static void HandleProgress(const HttpContext& context, QNetworkReply* reply, qint64 ready, qint64 total);

    bool use_json_;
    bool use_internal_;
    int32_t timeout_;
    QString json_;
    QUrlQuery params_;
    QHash<QString, QString> headers_;
    QString url_;
    QString charset_;
    QString user_agent_;
    QNetworkAccessManager *manager_;
    std::function<void (const QString &)> success_handler_;
    std::function<void(const QString&)> error_handler_;
    std::function<void (const QByteArray &)> download_handler_;
    std::function<void(qint64, qint64)> progress_handler_;
    LoggerPtr logger_;
};

HttpClient::HttpClientImpl::~HttpClientImpl() = default;

HttpClient::HttpClientImpl::HttpClientImpl(const QString &url, QObject* parent)
    : use_json_(false)
    , use_internal_(true)
    , timeout_(kHttpDefaultTimeout)
    , url_(url)
    , charset_(kDefaultCharset)
    , manager_(new QNetworkAccessManager(parent)) {
    logger_ = LoggerManager::GetInstance().GetLogger(kHttpLoggerName);
}

HttpContext HttpClient::HttpClientImpl::CreateHttpContext() const {
    HttpContext context;
    context.success_handler = success_handler_;
    context.error_handler = error_handler_;
    context.progress_handler = progress_handler_;
    context.manager = manager_;
    context.charset = charset_;
    context.user_agent = user_agent_;
    context.use_internal = use_internal_;
    context.logger = logger_;
    return context;
}

void HttpClient::HttpClientImpl::SetTimeout(int timeout) {
    timeout_ = timeout;
}

void HttpClient::HttpClientImpl::ExecuteQuery(QSharedPointer<HttpClientImpl> d, HttpMethod method) {
	auto context = d->CreateHttpContext();

    context.manager->setProxy(QNetworkProxy::NoProxy);

    const auto request = CreateHttpRequest(d, method);
    QNetworkReply *reply = nullptr;

    auto operation = QNetworkAccessManager::UnknownOperation;

    switch (method) {
    case HttpMethod::HTTP_GET:
        reply = context.manager->get(request);
        operation = QNetworkAccessManager::GetOperation;
        break;
    case HttpMethod::HTTP_POST:
        reply = context.manager->post(request,
                                      d->use_json_
                                      ? d->json_.toUtf8()
                                      : d->params_.toString(QUrl::FullyEncoded).toUtf8());
        operation = QNetworkAccessManager::PostOperation;
        break;
    case HttpMethod::HTTP_PUT:
        reply = context.manager->put(request,
                                     d->use_json_
                                     ? d->json_.toUtf8()
                                     : d->params_.toString(QUrl::FullyEncoded).toUtf8());
        operation = QNetworkAccessManager::PutOperation;
        break;
    case HttpMethod::HTTP_DELETE:
        reply = context.manager->deleteResource(request);
        operation = QNetworkAccessManager::DeleteOperation;
        break;
    }

    LogHttpRequest(context.logger, RequestVerb(operation, request), request);

    (void) QObject::connect(reply,
        &QNetworkReply::downloadProgress,
        [reply, context, d](auto ready, auto total) {
            HandleProgress(context, reply, ready, total);
        });

    (void) QObject::connect(reply,
        &QNetworkReply::finished,
        [reply, context, request, operation, d] {
        LogHttpRequest(context.logger, RequestVerb(operation, request), request, reply);
	    const auto success_message = ReadReply(reply, context.charset);
	    HandleFinish(context, reply, success_message);
    });
}

void HttpClient::HttpClientImpl::HandleProgress(const HttpContext& context, QNetworkReply* reply, qint64 ready, qint64 total) {
    const auto status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    const auto status = status_code.isValid() ? status_code.toInt() : 200;

    if (total > 0) {
        XAMP_LOG_D(context.logger, "Download progress: {}%", Round(ready * 100.0 / total));
    }

    if (status == 301 || status == 302) {
        return;
    }

    if (status != 200 && status != 206 && status != 416) {
        HandleFinish(context, reply, kEmptyString);
        return;
    }

    if (context.progress_handler != nullptr) {
        context.progress_handler(ready, total);
    }
}

void HttpClient::HttpClientImpl::download(QSharedPointer<HttpClientImpl> d, std::function<void (const QByteArray &)> ready_read) {
    auto context = d->CreateHttpContext();

    auto request = CreateHttpRequest(d, HttpMethod::HTTP_GET);
    auto* reply = context.manager->get(request);

    (void) QObject::connect(reply,
        &QNetworkReply::readyRead, 
        [reply, d, ready_read] {
        ready_read(reply->readAll());
    });

    (void) QObject::connect(reply,
        &QNetworkReply::finished,
        [reply, request, context, d] {
        LogHttpRequest(context.logger, RequestVerb(QNetworkAccessManager::GetOperation, request), request, reply);
        HandleFinish(context, reply, QString());
    });

    (void) QObject::connect(reply,
        &QNetworkReply::downloadProgress,
        [reply, context, d](auto ready, auto total) {
            HandleProgress(context, reply, ready, total);
        });
}

void HttpClient::HttpClientImpl::HandleFinish(const HttpContext &context, QNetworkReply *reply, const QString &success_message) {
    const auto error = reply->error();

    if (error == QNetworkReply::NoError) {
        if (context.success_handler != nullptr) {
            context.success_handler(success_message);
        }
    } else {
        if (context.error_handler != nullptr) {
            context.error_handler(NetworkErrorToString(error));
        }
    }

    if (reply != nullptr) {
        reply->deleteLater();
    }

    if (context.use_internal) {
        if (context.manager != nullptr) {
            context.manager->deleteLater();
        }
    }

    XAMP_LOG_D(context.logger, "Request finished! error: {}", NetworkErrorToString(error).data());
}

QString HttpClient::HttpClientImpl::ReadReply(QNetworkReply *reply, const QString &charset) {
    QScopedPointer<QTextStream> in;
    const auto content = reply->readAll();
    try
    {
        if (IsZipEncoding(reply)) {
            in.reset(new QTextStream(GzipDecompress(content)));
        }
        else {
            in.reset(new QTextStream(content));
        }
    }
    catch (...)
    {
        in.reset(new QTextStream(content));
    }

    const auto content_length_var = reply->header(QNetworkRequest::ContentLengthHeader);
    auto content_length = kHttpBufferSize;
    if (content_length_var.isValid()) {
        content_length = content_length_var.toInt();
    }

    QString result;
    result.reserve(content_length);
    in->setEncoding(QStringConverter::Utf8);

    while (!in->atEnd()) {
        result.append(in->readLine());
    }

    return result;
}

QNetworkRequest HttpClient::HttpClientImpl::CreateHttpRequest(QSharedPointer<HttpClientImpl> d, HttpMethod method) {
	const auto get = method == HttpMethod::HTTP_GET;
	const auto with_form = !get && !d->use_json_;
	const auto with_json = !get &&  d->use_json_;

    if (get && !d->params_.isEmpty()) {
        d->url_ += qTEXT("?") + d->params_.toString(QUrl::FullyEncoded);
    }

    if (with_form) {
        d->headers_[qTEXT("Content-Type")] = qTEXT("application/x-www-form-urlencoded");
    } else if (with_json) {
        d->headers_[qTEXT("Content-Type")] = qTEXT("application/json; charset=utf-8");
    }

    if (d->user_agent_.isEmpty()) {
        d->headers_[qTEXT("User-Agent")] = kDefaultUserAgent;
    }

    XAMP_LOG_D(d->logger_, "Request url: {}", QUrl(d->url_).toEncoded().toStdString());

    QNetworkRequest request(QUrl(d->url_));
    for (auto i = d->headers_.cbegin(); i != d->headers_.cend(); ++i) {
        request.setRawHeader(i.key().toUtf8(), i.value().toUtf8());
    }

    if (request.url().scheme() == qTEXT("https")) {
        static const bool http2_enabled_env = qEnvironmentVariableIntValue("OWNCLOUD_HTTP2_ENABLED") == 1;
        request.setAttribute(QNetworkRequest::Http2AllowedAttribute, http2_enabled_env);
    }

    const auto request_id = GenerateUuid();

    request.setRawHeader("X-Request-ID", request_id);

    // todo: Add GZIP support.
    /*if (isLoadZib()) {
        request.setRawHeader("Accept-Encoding", "gzip");
        XAMP_LOG_D(d->logger_, "Use gzip compression.");
    }*/

    request.setTransferTimeout(d->timeout_);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    return request;
}

HttpClient::HttpClient(const QUrl& url, QObject* parent)
    : HttpClient(url.toString(), parent) {
}

HttpClient::HttpClient(const QString &url, QObject* parent)
    : impl_(new HttpClientImpl(url, parent)) {
}

HttpClient::~HttpClient() = default;

void HttpClient::setTimeout(int timeout) {
    impl_->SetTimeout(timeout);
}

HttpClient& HttpClient::param(const QString &name, const QVariant &value) {
    impl_->params_.addQueryItem(name, value.toString());
    return *this;
}

HttpClient& HttpClient::params(const QMap<QString, QVariant> &ps) {
    for (auto itr = ps.cbegin(); itr != ps.cend(); ++itr) {
        impl_->params_.addQueryItem(itr.key(), itr.value().toString());
    }
    return *this;
}

HttpClient& HttpClient::json(const QString &json) {
    impl_->json_    = json;
    impl_->use_json_ = true;
    return *this;
}

HttpClient& HttpClient::header(const QString &name, const QString &value) {
    impl_->headers_[name] = value;
    return *this;
}

HttpClient& HttpClient::headers(const QMap<QString, QString> name_values) {
    for (auto i = name_values.cbegin(); i != name_values.cend(); ++i) {
        impl_->headers_[i.key()] = i.value();
    }
    return *this;
}

HttpClient& HttpClient::success(std::function<void (const QString &)> success_handler) {
    impl_->success_handler_ = std::move(success_handler);
    return *this;
}

HttpClient& HttpClient::error(std::function<void(const QString&)> error_handler) {
    impl_->error_handler_ = std::move(error_handler);
    return *this;
}

HttpClient& HttpClient::progress(std::function<void(qint64, qint64)> progressHandler) {
    impl_->progress_handler_ = std::move(progressHandler);
    return *this;
}

void HttpClient::get() {
    HttpClientImpl::ExecuteQuery(impl_, HttpMethod::HTTP_GET);
}

void HttpClient::post() {
    HttpClientImpl::ExecuteQuery(impl_, HttpMethod::HTTP_POST);
}

void HttpClient::downloadFile(const QString& file_name, std::function<void(const QString&)> download_handler, std::function<void(const QString&)> error_handler) {
    QSharedPointer<QTemporaryFile> tempfile(new QTemporaryFile());
    if (!tempfile->open()) {
        return;
    }

    tempfile->setAutoRemove(false);

    success([=](auto) {
        tempfile->rename(file_name);
        download_handler(file_name);
        });

    error(std::move(error_handler));

    HttpClientImpl::download(impl_, [tempfile](auto buffer) {
        tempfile->write(buffer);
        });
}

void HttpClient::download(std::function<void (const QByteArray &)> download_handler, std::function<void(const QString&)> error_handler) {
    auto data = std::make_shared<QByteArray>();

    success([handler = std::move(download_handler), data](auto) {
        handler(*data);
    });

    error(std::move(error_handler));

    HttpClientImpl::download(impl_, [data](auto buffer) {
        data->append(buffer);
    });
}

void HttpClient::setUserAgent(const QString& user_agent) {
    impl_->user_agent_ = user_agent;
}

}
