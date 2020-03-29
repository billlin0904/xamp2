//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <mutex>

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
    explicit MetadataExtractAdapter(QObject* parent = nullptr);

	~MetadataExtractAdapter() override;

    void OnWalk(const xamp::metadata::Path& path, xamp::base::Metadata metadata) override;

	void OnWalkNext() override;

    bool IsCancel() const override;

	void Cancel() override;

	void Reset() override;	

signals:
	void readCompleted(std::vector<xamp::base::Metadata> enitiy);

public:
	static void processMetadata(const std::vector<xamp::base::Metadata>& metadatas, PlayListTableView *playlist = nullptr);
	
private:
    bool cancel_;
	std::vector<xamp::base::Metadata> metadatas_;    
};

