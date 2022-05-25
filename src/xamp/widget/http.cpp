#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QUrl>
#include <QTemporaryFile>

#include <memory>
#include <base/logger.h>
#include <widget/str_utilts.h>
#include <widget/http.h>

namespace http {

struct HttpContext {
    bool use_internal;
    QString charset;
    QString user_agent;
    QNetworkAccessManager* manager;
    std::function<void (const QString &)> success_handler;
    std::function<void(const QString&)> error_handler;
};

class HttpClient::HttpClientImpl {
public:
    HttpClientImpl(const QString &url, QNetworkAccessManager* manager);

    HttpContext createContext() const;

    static QNetworkRequest createRequest(HttpClientImpl *d, HttpMethod method);

    static void executeQuery(HttpClientImpl *d, HttpMethod method);

    static void download(HttpClientImpl *d, std::function<void (const QByteArray &)> readyRead);

    static QString readReply(QNetworkReply *reply, const QString &charset);

    static void handleFinish(HttpContext context, QNetworkReply *reply, const QString &successMessage);

    bool use_json_;
    bool use_internal_;
    QString url_;
    QString json_;
    QUrlQuery params;
    QString charset_;
    QString user_agent_;
    QNetworkAccessManager *manager_;
    QHash<QString, QString> headers_;
    std::function<void (const QString &)> success_handler_;
    std::function<void(const QString&)> error_handler_;
    std::function<void (const QByteArray &)> download_handler_;
};

HttpClient::HttpClientImpl::HttpClientImpl(const QString &url, QNetworkAccessManager* manager)
    : use_json_(false)
    , use_internal_(manager == nullptr)
    , url_(url)
    , charset_(Q_TEXT("UTF-8"))
    , manager_(manager == nullptr ? new QNetworkAccessManager() : manager) {
}

HttpContext HttpClient::HttpClientImpl::createContext() const {
    HttpContext context;
    context.success_handler = success_handler_;
    context.error_handler = error_handler_;
    context.manager = manager_;
    context.charset = charset_;
    context.user_agent = user_agent_;
    context.use_internal = use_internal_;
    return context;
}

void HttpClient::HttpClientImpl::executeQuery(HttpClientImpl *d, HttpMethod method) {
    auto context = d->createContext();

    const auto request = createRequest(d, method);
    QNetworkReply *reply = nullptr;

    switch (method) {
    case HttpMethod::HTTP_GET:
        reply = context.manager->get(request);
        break;
    case HttpMethod::HTTP_POST:
        reply = context.manager->post(request,
                                      d->use_json_
                                      ? d->json_.toUtf8()
                                      : d->params.toString(QUrl::FullyEncoded).toUtf8());
        break;
    case HttpMethod::HTTP_PUT:
        reply = context.manager->put(request,
                                     d->use_json_
                                     ? d->json_.toUtf8()
                                     : d->params.toString(QUrl::FullyEncoded).toUtf8());
        break;
    case HttpMethod::HTTP_DELETE:
        reply = context.manager->deleteResource(request);
        break;
    }

    (void) QObject::connect(reply, &QNetworkReply::finished, [=] {
	    const auto success_message = readReply(reply,
	                                           QString::fromStdString(context.charset.toUtf8().toStdString()));
	    handleFinish(context, reply, success_message);
    });
}

void HttpClient::HttpClientImpl::download(HttpClientImpl *d, std::function<void (const QByteArray &)> readyRead) {
    auto context = d->createContext();

    auto request = createRequest(d, HttpMethod::HTTP_GET);
    auto* reply = context.manager->get(request);

    (void) QObject::connect(reply, &QNetworkReply::readyRead, [=] {
        readyRead(reply->readAll());
    });

    (void) QObject::connect(reply, &QNetworkReply::finished, [=] {
        handleFinish(context, reply, QString());
    });
}

void HttpClient::HttpClientImpl::handleFinish(HttpContext context, QNetworkReply *reply, const QString &successMessage) {
    if (reply->error() == QNetworkReply::NoError) {
        if (context.success_handler != nullptr) {
            context.success_handler(successMessage);
        }
    } else {
        if (context.error_handler != nullptr) {
            context.error_handler(successMessage);
        }
        XAMP_LOG_DEBUG("{}", successMessage.toStdString());
    }

    if (reply != nullptr) {
        reply->deleteLater();
    }

    if (context.use_internal) {
        if (context.manager != nullptr) {
            context.manager->deleteLater();
        }
    }
}

QString HttpClient::HttpClientImpl::readReply(QNetworkReply *reply, const QString &charset) {
    QTextStream in(reply);

    QString result;
    in.setCodec(charset.toUtf8());

    while (!in.atEnd()) {
        result += in.readLine();
    }

    return result;
}

QNetworkRequest HttpClient::HttpClientImpl::createRequest(HttpClientImpl *d, HttpMethod method) {
    auto get = method == HttpMethod::HTTP_GET;
    auto with_form = !get && !d->use_json_;
    auto with_json = !get &&  d->use_json_;

    if (get && !d->params.isEmpty()) {
        d->url_ += Q_TEXT("?") + d->params.toString(QUrl::FullyEncoded);
    }

    if (with_form) {
        d->headers_[Q_TEXT("Content-Type")] = Q_TEXT("application/x-www-form-urlencoded");
    } else if (with_json) {
        d->headers_[Q_TEXT("Content-Type")] = Q_TEXT("application/json; charset=utf-8");
    }

    if (d->user_agent_.isEmpty()) {
        d->headers_[Q_TEXT("User-Agent")] = Q_TEXT("xamp-player/1.0.0");
    }

    XAMP_LOG_DEBUG("Request url: {}", QUrl(d->url_).toEncoded().toStdString());

    QNetworkRequest request(QUrl(d->url_));
    for (auto i = d->headers_.cbegin(); i != d->headers_.cend(); ++i) {
        request.setRawHeader(i.key().toUtf8(), i.value().toUtf8());
    }

    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    return request;
}

HttpClient::HttpClient(const QUrl& url, QNetworkAccessManager* manager)
    : HttpClient(url.toString(), manager) {
}

HttpClient::HttpClient(const QString &url, QNetworkAccessManager* manager)
    : impl_(new HttpClientImpl(url, manager)) {
}

HttpClient::~HttpClient() = default;

HttpClient& HttpClient::param(const QString &name, const QVariant &value) {
    impl_->params.addQueryItem(name, value.toString());
    return *this;
}

HttpClient& HttpClient::params(const QMap<QString, QVariant> &ps) {
    for (auto itr = ps.cbegin(); itr != ps.cend(); ++itr) {
        impl_->params.addQueryItem(itr.key(), itr.value().toString());
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

HttpClient& HttpClient::headers(const QMap<QString, QString> nameValues) {
    for (auto i = nameValues.cbegin(); i != nameValues.cend(); ++i) {
        impl_->headers_[i.key()] = i.value();
    }
    return *this;
}

HttpClient& HttpClient::success(std::function<void (const QString &)> successHandler) {
    impl_->success_handler_ = successHandler;
    return *this;
}

HttpClient& HttpClient::error(std::function<void(const QString&)> errorHandler) {
    impl_->error_handler_ = errorHandler;
    return *this;
}

void HttpClient::get() {
    HttpClientImpl::executeQuery(impl_.get(), HttpMethod::HTTP_GET);
}

void HttpClient::post() {
    HttpClientImpl::executeQuery(impl_.get(), HttpMethod::HTTP_POST);
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

    error(error_handler);

    HttpClientImpl::download(impl_.get(), [tempfile](auto buffer) {
        tempfile->write(buffer);
        });
}

void HttpClient::download(std::function<void (const QByteArray &)> download_handler, std::function<void(const QString&)> error_handler) {
    auto data = std::make_shared<QByteArray>();

    success([=](auto) {
        download_handler(*data);
    });

    error(error_handler);

    HttpClientImpl::download(impl_.get(), [data](auto buffer) {
        data->append(buffer);
    });
}

void HttpClient::setUserAgent(const QString& user_agent) {
    impl_->user_agent_ = user_agent;
}

}
