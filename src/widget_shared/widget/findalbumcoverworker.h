//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

#include <widget/widget_shared_global.h>

class XAMP_WIDGET_SHARED_EXPORT FindAlbumCoverWorker : public QObject {
	Q_OBJECT
public:
	FindAlbumCoverWorker();

signals:
	void SetAlbumCover(int32_t album_id,
		const QString& album, 
		const QString& cover_id);

public slots:
	void OnFindAlbumCover(int32_t album_id,
		const QString& album,
		const std::wstring& file_path);

	void OnCancelRequested();
private:
	bool is_stop_{ true };
};

