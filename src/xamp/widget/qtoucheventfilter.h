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