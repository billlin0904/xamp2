//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <atomic>

#include <QObject>

#include <metadata/metadatareader.h>
#include <metadata/metadataextractadapter.h>

#include <widget/widget_shared.h>
#include <widget/pixmapcache.h>

class PlayListTableView;

class MetadataExtractAdapter final
	: public QObject {
	Q_OBJECT
public:
    explicit MetadataExtractAdapter(QObject* parent = nullptr);

	~MetadataExtractAdapter() override;

    static void ReadFileMetadata(MetadataExtractAdapter* adapter, QString const& file_name);

signals:
	void readCompleted(std::vector<Metadata> enitiy);

public:
    static void ProcessMetadata(const std::vector<Metadata>& result, PlayListTableView *playlist = nullptr);	
};

