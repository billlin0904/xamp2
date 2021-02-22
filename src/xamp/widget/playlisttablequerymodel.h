//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QSqlQueryModel>

class PlayListTableQueryModel final : public QSqlQueryModel {
public:
	explicit PlayListTableQueryModel(int32_t playlist_id, QObject* parent = nullptr);
	
	void refreshOnece();
private:
	int32_t playlist_id_;
};
