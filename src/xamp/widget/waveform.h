//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>

#include <QWidget>
#include <QPixmap>
#include <QScopedPointer>

#include <widget/widget_shared.h>

enum class FileHandlingMode { 
    FULL_CACHE,
    DISK_MODE
};

enum class DrawingMode {
    OVERVIEW,
    MACRO, 
    NO_MODE
};

class Waveform final : public QWidget {
public:
	Waveform(QWidget* parent = nullptr);

    void setColor(QColor color) {
        waveformColor_ = color;
    }

    void setFileHandlingMode(FileHandlingMode mode) {
        currentFileHandlingMode_ = mode;
    }

protected:
	void paintEvent(QPaintEvent* event) override;

	void resizeEvent(QResizeEvent* event) override;

private:
    void recalculatePeaks();

    DrawingMode currentDrawingMode_;
    FileHandlingMode currentFileHandlingMode_;
    double max_peak_;
    double padding_;
    double scaleFactor_;    
    QSize lastSize_;
    QColor waveformColor_;
    std::vector<double> peeks_;
    std::vector<double> datas_;
};
