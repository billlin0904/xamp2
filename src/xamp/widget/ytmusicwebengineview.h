//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QWebEngineView>

class YtMusicWebEngineView : public QWebEngineView {
	Q_OBJECT
public:
	explicit YtMusicWebEngineView(QWidget* parent);

	void indexPage();

public slots:
	void loadFinished(bool ok);

private:
	void injectCustomCSS();
};
