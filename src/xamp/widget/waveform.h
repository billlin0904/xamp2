//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>

#include <QWidget>
#include <QPixmap>
#include <QScopedPointer>

#include <base/align_ptr.h>
#include <widget/widget_shared.h>

class Waveform final : public QWidget {
public:
	Waveform(QWidget* parent = nullptr);

    void setColor(QColor color) {
        waveformColor_ = color;
    }

    void setFileHandlingMode(FileHandlingMode mode) {
        currentFileHandlingMode_ = mode;
    }

    void load(const std::wstring & file_ext);

protected:
	void paintEvent(QPaintEvent* event) override;

	void resizeEvent(QResizeEvent* event) override;

private:
    void establishDrawingMode();

    void overviewDraw(QPaintEvent* event);

    void recalculatePeaks();

    std::vector<float> peakForRegion(int region_start_frame, int region_end_frame);

    float max_peak_;
    float padding_;
    float scaleFactor_;    
    QSize lastSize_;
    QColor waveformColor_;
    AlignPtr<FileStream> stream_;
    std::vector<double> peeks_;
    std::vector<double> datas_;
};