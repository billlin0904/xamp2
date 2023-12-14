//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
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
	explicit GenreView(QWidget* parent = nullptr);

	void setGenre(const QString& genre);

	void showAll() override;

	void showAllAlbum(int32_t limit = kMaxShowAlbum);
private:
	void resizeEvent(QResizeEvent* event) override;

	bool show_all_{ false };
	QString genre_;
};