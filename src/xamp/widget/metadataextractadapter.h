//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <metadata/metadataextractadapter.h>
#include <widget/pixmapcache.h>

class PlayListTableView;

class MetadataExtractAdapter final
	: public QObject {
	Q_OBJECT
public:
    explicit MetadataExtractAdapter(QObject* parent = nullptr);

	~MetadataExtractAdapter() override;

    static void ReadFileMetadata(const QSharedPointer<MetadataExtractAdapter>& adapter, QString const& file_name);

signals:
	void readCompleted(const std::vector<Metadata> &entity);

public:
    static void ProcessMetadata(const std::vector<Metadata>& result, PlayListTableView *playlist = nullptr);	
};

