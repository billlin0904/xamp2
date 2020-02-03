//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QTouchEvent>

class QTouchEventFilter : public QObject {
	Q_OBJECT
public:
	QTouchEventFilter(QObject* parent = nullptr);

protected:
	bool eventFilter(QObject* p_obj, QEvent* p_event);
};