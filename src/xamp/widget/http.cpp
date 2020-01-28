#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QUrl>

#include <memory>
#include <base/logger.h>
#include <widget/str_utilts.h>
#include <widget/http.h>

namespace http {

struct HttpContext {
    QString charset;
    QNetworkAccessManager* manager;
    std::function<void (const QString &)> successHandler;
};

class HttpClient::HttpClientImpl {
public:
    explicit HttpClientImpl(const QString &url);

    HttpContext makeContext();

    static QNetworkRequest createRequest(HttpClientImpl *d, HttpMethod method);

    static void executeQuery(HttpClientImpl *d, HttpMethod method);

    static void download(HttpClientImpl *d, std::function<void (const QByteArray &)> readyRead);

    static QString readReply(QNetworkReply *reply, const QString &charset);

    static void handleFinish(HttpContext context, QNetworkReply *reply, const QString &successMessage);

    bool useJson;
    QString url;
    QString json;
    QUrlQuery params;
    QString charset;
    QNetworkAccessManager *manager;
    QHash<QString, QString> headers;
    std::function<void (const QString &)> successHandler;
    std::function<void (const QByteArray &)> downloadHandler;
};

HttpClient::HttpClientImpl::HttpClientImpl(const QString &url)
    : useJson(false)
    , url(url)
    , charset(Q_UTF8("UTF-8"))
    , manager(new QNetworkAccessManager()) {
}

HttpContext HttpClient::HttpClientImpl::makeContext() {
    HttpContext context;
    context.successHandler = successHandler;
    context.manager = manager;
    context.charset = charset;
    return context;
}

void HttpClient::HttpClientImpl::executeQuery(HttpClientImpl *d, HttpMethod method) {
    auto context = d->makeContext();

    auto request = HttpClientImpl::createRequest(d, method);
    QNetworkReply *reply = nullptr;

    switch (method) {
    case HttpMethod::GET:
        reply = context.manager->get(request);
        break;
    case HttpMethod::POST:
        reply = context.manager->post(request,
                                      d->useJson
                                      ? d->json.toUtf8()
                                      : d->params.toString(QUrl::FullyEncoded).toUtf8());
        break;
    case HttpMethod::PUT:
        reply = context.manager->put(request,
                                     d->useJson
                                     ? d->json.toUtf8()
                                     : d->params.toString(QUrl::FullyEncoded).toUtf8());
        break;
    case HttpMethod::DELETE:
        reply = context.manager->deleteResource(request);
        break;
    }

    (void) QObject::connect(reply, &QNetworkReply::finished, [=] {
        auto successMessage = HttpClientImpl::readReply(reply,
             QString::fromStdString(context.charset.toUtf8().toStdString()));
        HttpClientImpl::handleFinish(context, reply, successMessage);
    });
}

void HttpClient::HttpClientImpl::download(HttpClientImpl *d, std::function<void (const QByteArray &)> readyRead) {
    auto context = d->makeContext();

    auto request = HttpClientImpl::createRequest(d, HttpMethod::GET);
    auto reply = context.manager->get(request);

    (void) QObject::connect(reply, &QNetworkReply::readyRead, [=] {
        readyRead(reply->readAll());
    });

    (void) QObject::connect(reply, &QNetworkReply::finished, [=] {
        HttpClientImpl::handleFinish(context, reply, QString());
    });
}

void HttpClient::HttpClientImpl::handleFinish(HttpContext context, QNetworkReply *reply, const QString &successMessage) {
    if (reply->error() == QNetworkReply::NoError) {
        if (context.successHandler != nullptr) {
            context.successHandler(successMessage);
        }
    } else {
        XAMP_LOG_DEBUG("{}", successMessage.toStdString());
    }

    if (reply != nullptr) {
        reply->deleteLater();
    }

    if (context.manager != nullptr) {
        context.manager->deleteLater();
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
    auto get = method == HttpMethod::GET;
    auto upload = method == HttpMethod::UPLOAD;
    auto withForm = !get && !upload && !d->useJson;
    auto withJson = !get && !upload &&  d->useJson;

    if (get && !d->params.isEmpty()) {
        d->url += Q_UTF8("?") + d->params.toString(QUrl::FullyEncoded);
    }

    if (withForm) {
        d->headers[Q_UTF8("Content-Type")] = Q_UTF8("application/x-www-form-urlencoded");
    } else if (withJson) {
        d->headers[Q_UTF8("Content-Type")] = Q_UTF8("application/json; charset=utf-8");
    }

    QNetworkRequest request(QUrl(d->url));
    for (auto i = d->headers.cbegin(); i != d->headers.cend(); ++i) {
        request.setRawHeader(i.key().toUtf8(), i.value().toUtf8());
    }
    return request;
}

HttpClient::HttpClient(const QString &url)
    : impl_(new HttpClientImpl(url)) {
}

HttpClient::~HttpClient() {
}

HttpClient& HttpClient::param(const QString &name, const QVariant &value) {
    impl_->params.addQueryItem(name, value.toString());
    return *this;
}

HttpClient& HttpClient::params(const QMap<QString, QVariant> &ps) {
    for (auto iter = ps.cbegin(); iter != ps.cend(); ++iter) {
        impl_->params.addQueryItem(iter.key(), iter.value().toString());
    }
    return *this;
}

HttpClient& HttpClient::json(const QString &json) {
    impl_->json    = json;
    impl_->useJson = true;
    return *this;
}

HttpClient& HttpClient::header(const QString &name, const QString &value) {
    impl_->headers[name] = value;
    return *this;
}

HttpClient& HttpClient::headers(const QMap<QString, QString> nameValues) {
    for (auto i = nameValues.cbegin(); i != nameValues.cend(); ++i) {
        impl_->headers[i.key()] = i.value();
    }
    return *this;
}

HttpClient& HttpClient::success(std::function<void (const QString &)> successHandler) {
    impl_->successHandler = successHandler;
    return *this;
}

void HttpClient::get() {
    HttpClientImpl::executeQuery(impl_.get(), HttpMethod::GET);
}

void HttpClient::post() {
    HttpClientImpl::executeQuery(impl_.get(), HttpMethod::POST);
}

void HttpClient::download(std::function<void (const QByteArray &)> downloadHandler) {
    auto data = std::make_shared<QByteArray>();

    success([=](auto message) {
        downloadHandler(*data);
    });

    HttpClientImpl::download(impl_.get(), [data](auto buffer) {
        data->append(buffer);
    });
}

}
