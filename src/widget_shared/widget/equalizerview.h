//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>

#include <base/buffer.h>
#include <base/memory.h>
#include <QFrame>
#include <QString>

#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>
#include <stream/eqsettings.h>

namespace xamp::stream {
    class BassParametricEq;
    class STFT;
}

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSlider;
class QSpinBox;
class QTimer;

class ParametricEqGraph;

namespace Ui {
    class EqualizerView;
}

class XAMP_WIDGET_SHARED_EXPORT EqualizerView final : public QFrame {
    Q_OBJECT
public:
    explicit EqualizerView(QWidget *parent = nullptr);

    virtual ~EqualizerView() override;

signals:
   void bandValueChanged(int band, float value, float Q);

   void preampValueChanged(float value);

   void parametricEqChanged(bool enabled, const EqSettings& settings);

public slots:
   void outputFormatChanged(int32_t sample_rate, size_t buffer_size);

   void samplesChanged(std::vector<float> samples, size_t num_samples);

private:
    void applySetting(const QString &name, const EqSettings &settings);

    void rebuildBandRows();

    void updateBandFromUi(size_t band);

    void updateBandFromGraph(size_t band, float frequency, float gain);

    void updateGraph();

    void saveCurrentSetting();

    EqSettings currentEnabledSettings() const;

    void updatePreampControl();

    void updateBandGainControls();

    float effectivePreamp() const;

    void resetAnalyzer();

    void configureAnalyzer(size_t num_samples);

    void toggleTestWaveformGeneration();

    void generateTestWaveform();

    void resetTestEq();

    void configureTestEq(const EqSettings& settings);

    void applyTestEq(std::vector<float>& samples);

    struct BandControls {
        QCheckBox* enabled{ nullptr };
        QComboBox* curve{ nullptr };
        QDoubleSpinBox* gain{ nullptr };
        QDoubleSpinBox* q{ nullptr };
        QSpinBox* frequency{ nullptr };
        QLabel* color{ nullptr };
    };

    QString current_name_;
    EqSettings current_settings_;
    std::vector<BandControls> band_controls_;
    ParametricEqGraph* graph_{ nullptr };
    QCheckBox* preamp_enabled_checkbox_{ nullptr };
    QSlider* preamp_slider_{ nullptr };
    QLabel* preamp_value_label_{ nullptr };
    QPushButton* apply_button_{ nullptr };
    QPushButton* test_wave_button_{ nullptr };
    QTimer* test_wave_timer_{ nullptr };
    std::vector<double> test_noise_state_;
    xamp::base::ScopedPtr<xamp::stream::BassParametricEq> test_eq_;
    Buffer<float> test_eq_buffer_;
    int32_t test_eq_sample_rate_{ 0 };
    xamp::base::ScopedPtr<xamp::stream::STFT> analyzer_stft_;
    int32_t analyzer_sample_rate_{ 0 };
    size_t analyzer_frame_size_{ 0 };
    size_t analyzer_shift_size_{ 0 };
    Ui::EqualizerView *ui_;
};
