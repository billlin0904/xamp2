//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/widget_shared_global.h>
#include <widget/albumview.h>

class XAMP_WIDGET_SHARED_EXPORT GenreView final : public AlbumView {
public:
	explicit GenreView(QWidget* parent = nullptr);

	void SetGenre(const QString& genre);

	void ShowAll() override;

	void ShowAllAlbum(int32_t limit = 65535);
private:
	QString genre_;
};