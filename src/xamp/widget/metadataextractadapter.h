//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <widget/widget_shared.h>

class PlayListTableView;

Metadata getMetadata(QString const& file_path);

class MetadataExtractAdapter final
	: public QObject {
	Q_OBJECT
public:
    explicit MetadataExtractAdapter(QObject* parent = nullptr);

    static void readFileMetadata(const QSharedPointer<MetadataExtractAdapter>& adapter, QString const& file_path, bool show_progress_dialog = true, bool is_recursive = true);

signals:
	void readCompleted(int64_t dir_last_write_time, const ForwardList<Metadata> &entity);

public:
    static void processMetadata(const ForwardList<Metadata>& result, PlayListTableView *playlist = nullptr, int64_t dir_last_write_time = -1);

	static void addMetadata(const ForwardList<Metadata>& result, PlayListTableView* playlist, int64_t dir_last_write_time, bool is_podcast);
};

