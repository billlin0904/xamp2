//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QSqlQueryModel>

class PlayListTableQueryModel final : public QSqlQueryModel {
public:
	explicit PlayListTableQueryModel(int32_t playlist_id, QObject* parent = nullptr);

	Qt::ItemFlags flags(const QModelIndex& index) const override;

	QVariant data(const QModelIndex& index, int role) const override;

	bool setData(const QModelIndex& index, const QVariant& value, int role) override;

	void selectAll(bool check);

	void refreshOnece();
private:
	void updateSelected(const QModelIndex& index, bool check);
	int32_t playlist_id_;
};
