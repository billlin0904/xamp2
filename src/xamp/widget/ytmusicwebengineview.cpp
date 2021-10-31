#include <base/logger.h>

#include <QFile>
#include <QWebEngineProfile>
#include <QWebChannel>
#include <QWebEngineCookieStore>
#include <QWebEngineSettings>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>

#include <widget/str_utilts.h>
#include <widget/ytmusicobserver.h>
#include <widget/ytmusicwebengineview.h>

YtMusicWebEngineView::YtMusicWebEngineView(QWidget* parent)
	: QWebEngineView(parent)
	, observer_(nullptr)
	, channel_(nullptr) {
}

bool YtMusicWebEngineView::isLoaded() const {
    return channel_ != nullptr;
}

void YtMusicWebEngineView::indexPage() {
#if QT_VERSION == QT_VERSION_CHECK(5, 15, 2)
    // https://bugreports.qt.io/browse/QTBUG-85382
    // todo: 可能還要清除 \AppData\Local\XAMP2 Project\XAMP2\QtWebEngine\Default底下的內容
    settings()->setAttribute(QWebEngineSettings::LocalStorageEnabled, false);
#endif
    injectQWebChannelJs();
    injectObserver();

    channel_ = new QWebChannel(this->page());
    observer_ = new YtMusicObserver(this);
    channel_->registerObject(Q_UTF8("observer"), observer_);
    this->page()->setWebChannel(channel_);

    load(QUrl(Q_UTF8("https://music.youtube.com")));
 
    (void)QObject::connect(this->page(), 
        &QWebEnginePage::loadFinished, this, &YtMusicWebEngineView::loadFinished);

    (void)QObject::connect(this->page(),
        &QWebEnginePage::featurePermissionRequested, this, &YtMusicWebEngineView::onFeaturePermissionRequested);
}

void YtMusicWebEngineView::onFeaturePermissionRequested(const QUrl& securityOrigin, QWebEnginePage::Feature feature) {
    this->page()->setFeaturePermission(securityOrigin, feature, QWebEnginePage::PermissionPolicy::PermissionDeniedByUser);
}

void YtMusicWebEngineView::loadFinished(bool) {
    injectCustomCSS();
}

void YtMusicWebEngineView::injectQWebChannelJs() {
    QWebEngineScript webChannelJs;
    webChannelJs.setInjectionPoint(QWebEngineScript::DocumentCreation);
    webChannelJs.setWorldId(QWebEngineScript::MainWorld);
    webChannelJs.setName(Q_UTF8("qwebchannel.js"));
    webChannelJs.setRunsOnSubFrames(true);
    {
        QFile webChannelJsFile(Q_UTF8(":/qtwebchannel/qwebchannel.js"));
        webChannelJsFile.open(QFile::ReadOnly);
        auto js = QString::fromUtf8(webChannelJsFile.readAll());
        webChannelJs.setSourceCode(js);
    }
    page()->scripts().insert(webChannelJs);
    XAMP_LOG_DEBUG("injectQWebChannelJs");
}

void YtMusicWebEngineView::injectObserver() {
    QWebEngineScript overrideJsPrint;
    overrideJsPrint.setInjectionPoint(QWebEngineScript::DocumentCreation);
    overrideJsPrint.setWorldId(QWebEngineScript::MainWorld);
    overrideJsPrint.setName(Q_UTF8("observer.js"));
    overrideJsPrint.setRunsOnSubFrames(true);
    {
        QFile file(Q_UTF8(":/xamp/Resource/inject/observer.js"));
        file.open(QIODevice::ReadOnly);
        auto js = QString::fromUtf8(file.readAll());
        overrideJsPrint.setSourceCode(js);
    }
    page()->scripts().insert(overrideJsPrint);
    XAMP_LOG_DEBUG("injectObserver");
}

void YtMusicWebEngineView::injectCustomCSS() {
    QFile file(Q_UTF8(":/xamp/Resource/inject/custom.css"));
    file.open(QIODevice::ReadOnly);

    auto js =
        Q_STR("var style = document.createElement('style'); style.innerHTML = ('%1'); document.head.appendChild(style);")
        .arg(QString::fromUtf8(file.readAll()));
    js.replace(Q_UTF8("\n"), Qt::EmptyString);
    js.replace(Q_UTF8("{"), Q_UTF8("\\{"));
    js.replace(Q_UTF8("}"), Q_UTF8("\\}"));

    page()->runJavaScript(js, [](const QVariant& result) {
        XAMP_LOG_DEBUG("injectCustomCSS result:{}", result.toString().toStdString());
        });
}
