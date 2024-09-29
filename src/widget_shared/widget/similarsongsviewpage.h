//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <widget/audio_embedding/audio_embedding_service.h>

class PlaylistTableView;

class XAMP_WIDGET_SHARED_EXPORT SimilarSongViewPage final : public QFrame {
	Q_OBJECT
public:
	explicit SimilarSongViewPage(QWidget* parent = nullptr);

	PlaylistTableView* playlist() const {
		return playlist_table_view_;
	}
signals:

public slots:
	void onQueryEmbeddingsReady(const QList<EmbeddingQueryResult>& results);

private:
	PlaylistTableView* playlist_table_view_;
};
