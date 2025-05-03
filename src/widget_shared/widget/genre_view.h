//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/widget_shared_global.h>
#include <widget/albumview.h>

class QResizeEvent;

inline constexpr auto kMaxShowAlbum = 65535;
inline constexpr auto kAlbumViewCoverSize = 215;

class XAMP_WIDGET_SHARED_EXPORT GenreView final : public AlbumView {
public:
	enum ViewType {
		VIEW_TYPE_CLOUD_PLAYLIST
	};

	explicit GenreView(QWidget* parent = nullptr);

	void setGenre(const QString& genre);

	void showAll() override;

private:
	void resizeEvent(QResizeEvent* event) override;

	void showAllAlbum(int32_t limit = kMaxShowAlbum);

	bool show_all_{ false };
	ViewType view_type_{ ViewType::VIEW_TYPE_CLOUD_PLAYLIST };
	QString genre_;
};