//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

#include <base/metadata.h>
#include <metadata/metadatareader.h>
#include <metadata/metadataextractadapter.h>
#include <metadata/taglibmetareader.h>

class PlayListTableView;

class MetadataExtractAdapter
	: public QObject
	, public xamp::metadata::MetadataExtractAdapter {
	Q_OBJECT
public:
	static const size_t PREALLOCATE_SIZE;

	MetadataExtractAdapter();

    void OnWalk(const xamp::metadata::Path& path, xamp::base::Metadata metadata) override;

	void OnWalkNext() override;

    bool IsCancel() const override;

	void Cancel() override;

	void Reset() override;

signals:
	void finish();

public:
	PlayListTableView* playlist;

private:
	void onCompleted(const std::vector<xamp::base::Metadata>& metadatas);

	bool cancel_;
	std::vector<xamp::base::Metadata> metadatas_;
    xamp::metadata::TaglibMetadataReader cover_reader_;
};

