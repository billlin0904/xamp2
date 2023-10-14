//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/widget_shared.h>
#include <QSqlQueryModel>

class LazyLoadingModel : public QSqlQueryModel {
public:
	static constexpr auto kMaxBatchSize = 200;

	explicit LazyLoadingModel(QObject* parent = nullptr)
		: QSqlQueryModel(parent) {
		batch_size_ = kMaxBatchSize;
		load_rows_ = 0;
	}

	[[nodiscard]] bool canFetchMore(const QModelIndex& parent = QModelIndex()) const override {
		return load_rows_ < rowCount();
	}

	void fetchMore(const QModelIndex& parent = QModelIndex()) override {
		QSqlQueryModel::fetchMore(parent);

		const int remaining_rows = rowCount() - load_rows_;
		const int rows_to_fetch = qMin(remaining_rows, batch_size_);
		if (rows_to_fetch <= 0) {
			return;
		}
		beginInsertRows(QModelIndex(), load_rows_, load_rows_ + rows_to_fetch - 1);
		load_rows_ += rows_to_fetch;
		endInsertRows();
	}

private:
	int32_t batch_size_;
	int32_t load_rows_;
};

