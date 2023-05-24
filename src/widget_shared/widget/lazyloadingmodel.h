//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QSqlQueryModel>

class LazyLoadingModel : public QSqlQueryModel {
public:
	static constexpr auto kMaxBatchSize = 64; // Max show 32 albums in one page

	explicit LazyLoadingModel(QObject* parent = nullptr)
		: QSqlQueryModel(parent) {
		batch_size_ = kMaxBatchSize;
		load_rows_ = 0;
	}

	bool canFetchMore(const QModelIndex& parent = QModelIndex()) const override {
		return load_rows_ < rowCount();
	}

	void fetchMore(const QModelIndex& parent = QModelIndex()) override {
		QSqlQueryModel::fetchMore(parent);

		int remainingRows = rowCount() - load_rows_;
		int rowsToFetch = qMin(remainingRows, batch_size_);
		if (rowsToFetch <= 0) {
			return;
		}
		beginInsertRows(QModelIndex(), load_rows_, load_rows_ + rowsToFetch - 1);
		load_rows_ += rowsToFetch;
		endInsertRows();
	}

private:
	int32_t batch_size_;
	int32_t load_rows_;
};

