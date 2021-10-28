#include <base/logger.h>

#include <QFile>
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>
#include <QWebEngineSettings>

#include <widget/str_utilts.h>
#include <widget/ytmusicwebengineview.h>

YtMusicWebEngineView::YtMusicWebEngineView(QWidget* parent)
	: QWebEngineView(parent) {
}

void YtMusicWebEngineView::indexPage() {
#if QT_VERSION == QT_VERSION_CHECK(5, 15, 2)
    // https://bugreports.qt.io/browse/QTBUG-85382
    // todo: 可能還要清除 \AppData\Local\XAMP2 Project\XAMP2\QtWebEngine\Default底下的內容
    settings()->setAttribute(QWebEngineSettings::LocalStorageEnabled, false);
#endif
    load(QUrl(Q_UTF8("https://music.youtube.com")));
    injectCustomCSS();

    (void)QObject::connect(this->page(), 
        &QWebEnginePage::loadFinished, this, &YtMusicWebEngineView::loadFinished);
}

void YtMusicWebEngineView::loadFinished(bool ok) {   
    injectCustomCSS();
}

void YtMusicWebEngineView::injectCustomCSS() {
    QFile file(Q_UTF8(":/xamp/Resource/inject/custom.css"));
    file.open(QIODevice::ReadOnly);

    auto js =
        Q_STR("var style = document.createElement('style'); style.innerHTML = ('%1'); document.head.appendChild(style);")
        .arg(QString::fromLatin1(file.readAll()));
    js.replace(Q_UTF8("\n"), Qt::EmptyString);
    js.replace(Q_UTF8("{"), Q_UTF8("\\{"));
    js.replace(Q_UTF8("}"), Q_UTF8("\\}"));

    page()->runJavaScript(js, [this](const QVariant& result) {
        XAMP_LOG_DEBUG("injectCustomCSS result:{}", result.toString().toStdString());
        });
}