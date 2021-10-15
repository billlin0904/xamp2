//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <metadata/metadataextractadapter.h>
#include <widget/pixmapcache.h>

class PlayListTableView;

Metadata getMetadata(QString const& file_path);

class MetadataExtractAdapter final
	: public QObject {
	Q_OBJECT
public:
    explicit MetadataExtractAdapter(QObject* parent = nullptr);

    static void readFileMetadata(const QSharedPointer<MetadataExtractAdapter>& adapter, QString const& file_path, bool show_progress_dialog = true);	

signals:
	void readCompleted(const std::vector<Metadata> &entity);

public:
    static void processMetadata(const std::vector<Metadata>& result, PlayListTableView *playlist = nullptr);
};

