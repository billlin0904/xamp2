//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QTimer>
#include <QWidget>

class Spectrograph : public QWidget {
    Q_OBJECT
public:
    explicit Spectrograph(QWidget* parent = nullptr);
	
public slots:
    void onDisplayChanged(std::vector<float> const& display);

private:
    void paintEvent(QPaintEvent* event) override;

    void updateBars();

    struct Bar {
        Bar()
    		: value(0.0f)
    		, clipped(false) {	        
        }
        float value;
        bool clipped;
    };

    QTimer timer_;
    QVector<Bar> bars_;
    std::vector<float> display_;
};


