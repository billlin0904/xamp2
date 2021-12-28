//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QWebEngineView>

class QWebChannel;
class YtMusicObserver;

class YtMusicWebEngineView : public QWebEngineView {
	Q_OBJECT
public:
	explicit YtMusicWebEngineView(QWidget* parent);

	bool isLoaded() const;

	void indexPage();

public slots:
	void loadFinished(bool ok);

	void onFeaturePermissionRequested(const QUrl& securityOrigin, QWebEnginePage::Feature feature);

private:
	void injectQWebChannelJs();

	void injectCustomCSS();

	void injectObserver();

	YtMusicObserver* observer_;
	QWebChannel* channel_;
};
