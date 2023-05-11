#include <widget/parametriceqview.h>

#include <array>

#include <widget/str_utilts.h>
#include <widget/smoothcurvegenerator2.h>
#include <stream/eqsettings.h>
#include <thememanager.h>

inline constexpr std::array<double, 11> kDBs = {
    -15, -12, -9, -6, -3, 0, 3, 6, 9, 12, 15
};

static uint32_t Index2Freq(int32_t i, uint32_t samples_rate, uint32_t fft_data_size) {
    return static_cast<uint32_t>(i) * (samples_rate / fft_data_size / 2.);
}

static uint32_t Freq2Index(uint32_t freq, uint32_t samples_rate, uint32_t fft_data_size) {
    return static_cast<uint32_t>(freq / (samples_rate / fft_data_size / 2.0));
}

const QStringList kBandsStr = QStringList()
<< qTEXT("31 Hz")
<< qTEXT("62 Hz")
<< qTEXT("125 Hz")
<< qTEXT("250 Hz")
<< qTEXT("500 Hz")
<< qTEXT("1 KHz")
<< qTEXT("2 KHz")
<< qTEXT("4 kHz")
<< qTEXT("8 kHz")
<< qTEXT("16 kHz");

const QStringList kDBStr = QStringList()
<< qTEXT("-15 dB")
<< qTEXT("-12 dB")
<< qTEXT("-9 dB")
<< qTEXT("-6 dB")
<< qTEXT("-3 dB")
<< qTEXT("0 dB")
<< qTEXT("+3 dB")
<< qTEXT("+6 dB")
<< qTEXT("+9 dB")
<< qTEXT("+12 dB")
<< qTEXT("+15 dB");

ParametricEqView::ParametricEqView(QWidget* parent) {
    const QColor gird_color(qTEXT("#495b6e"));
    const QColor tick_color(qTEXT("#5d7d9d"));
    const QColor tick_text_color(qTEXT("#a1a8af"));
    const QColor line_color(90, 123, 158);

    auto f = qTheme.GetMonoFont();
    f.setWeight(QFont::Light);
    f.setPointSize(qTheme.GetFontSize(5));
    //setOpenGl(true);    

    //xAxis->setScaleType(QCPAxis::stLogarithmic);
    setInteraction(QCP::Interaction::iRangeDrag, false);
    setInteraction(QCP::Interaction::iRangeZoom, false);
    yAxis->setRange(QCPRange(kEQMinDb, kEQMaxDb));
    xAxis->setRange(QCPRange(1, 15000));

    yAxis->setLabelFont(f);
    xAxis->setLabelFont(f);

    addGraph();
    addGraph();

    graph(dragable_graph_number_)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QPen(Qt::black, 1.5), QBrush(Qt::white), 9));
    graph(dragable_graph_number_)->setPen(QPen(line_color, 2));
    graph(dragable_graph_number_)->setSmooth();

    for (auto i = 0; i < kEqDefaultFrequencies.size() && i < kBandsStr.size(); i++) {
        graph(dragable_graph_number_)->addData(kEqDefaultFrequencies[i], 0);
    }

    const QSharedPointer<QCPAxisTickerText> fixed_ticker(new QCPAxisTickerText());
    xAxis->setTicker(fixed_ticker);
    fixed_ticker->setTickCount(kEqDefaultFrequencies.size());
    for (auto i = 0; i < kEqDefaultFrequencies.size() && i < kBandsStr.size(); i++) {
        fixed_ticker->addTick(kEqDefaultFrequencies[i], kBandsStr.at(i));
    }

    for (auto i = 0; i < kDBs.size(); i++) {
        graph(dragable_graph_number_)->addData(kDBs[i], 0);
    }

    const QSharedPointer<QCPAxisTickerText> y_fixed_ticker(new QCPAxisTickerText());
    yAxis->setTicker(y_fixed_ticker);
    y_fixed_ticker->setTickCount(kDBs.size());
    for (auto i = 0; i < kDBs.size(); i++) {
        y_fixed_ticker->addTick(kDBs[i], kDBStr.at(i));
    }

    xAxis->setBasePen(QPen(gird_color, 1));
    yAxis->setBasePen(QPen(gird_color, 1));
    xAxis->setTickPen(QPen(gird_color, 1));
    yAxis->setTickPen(QPen(gird_color, 1));
    xAxis->setSubTickPen(QPen(gird_color, 1));
    yAxis->setSubTickPen(QPen(gird_color, 1));
    xAxis->setTickLabelColor(tick_text_color);
    yAxis->setTickLabelColor(tick_text_color);
    xAxis->grid()->setPen(QPen(tick_color, 1, Qt::SolidLine));
    yAxis->grid()->setPen(QPen(tick_color, 1, Qt::SolidLine));
    xAxis->grid()->setSubGridPen(QPen(tick_color, 1, Qt::SolidLine));
    yAxis->grid()->setSubGridPen(QPen(tick_color, 1, Qt::SolidLine));
    xAxis->grid()->setSubGridVisible(true);
    yAxis->grid()->setSubGridVisible(true);
    xAxis->grid()->setZeroLinePen(Qt::NoPen);
    yAxis->grid()->setZeroLinePen(Qt::NoPen);
    
    /*QLinearGradient plot_gradient;
    plot_gradient.setStart(0, 0);
    plot_gradient.setFinalStop(0, 350);
    plot_gradient.setColorAt(0, QColor(80, 80, 80));
    plot_gradient.setColorAt(1, QColor(50, 50, 50));
    setBackground(plot_gradient);

    QLinearGradient axis_rect_gradient;
    axis_rect_gradient.setStart(0, 0);
    axis_rect_gradient.setFinalStop(0, 350);
    axis_rect_gradient.setColorAt(0, QColor(80, 80, 80));
    axis_rect_gradient.setColorAt(1, QColor(30, 30, 30));
    axisRect()->setBackground(axis_rect_gradient);*/
    setBackground(QColor(qTEXT("#0f1c2a")));    
}

void ParametricEqView::ClearBand() {
    xAxis->setTicker(QSharedPointer<QCPAxisTicker>(new QCPAxisTicker()));
    graph(dragable_graph_number_)->setData(QSharedPointer<QCPGraphDataContainer>(new QCPGraphDataContainer()));
}

void ParametricEqView::InitialAxisTicker(const EqSettings& settings) {
    const QSharedPointer<QCPAxisTickerText> fixed_ticker(new QCPAxisTickerText());
    xAxis->setTicker(fixed_ticker);
    fixed_ticker->setTickCount(settings.bands.size());

    QVector<double> bandwidths;
    for (auto i = 0; i < settings.bands.size(); i++) {
        fixed_ticker->addTick(settings.bands[i].frequency, FormatSampleRate(settings.bands[i].frequency));
        graph(dragable_graph_number_)->addData(settings.bands[i].frequency, 0);
        double bw = settings.bands[i].frequency / settings.bands[i].Q;
        bandwidths.append(bw);
    }

    auto* errorBars = new QCPErrorBars(xAxis, yAxis);
    errorBars->setData(bandwidths);
    errorBars->setSymbolGap(1);
    errorBars->setPen(QPen(QColor(qTEXT("#5d7d9d"))));
    errorBars->setWhiskerWidth(1.2);
    errorBars->setDataPlottable(graph(dragable_graph_number_));

    xAxis->setRange(QCPRange(settings.bands.front().frequency, settings.bands.back().frequency));
    xAxis->setScaleType(QCPAxis::stLinear);
}

void ParametricEqView::SetBand(EQFilterTypes type, int frequency, float value, float q) {
    
}

void ParametricEqView::SetSpectrumData(int frequency, float value) {
    auto graph_ptr = graph(0);
    for (auto itr = graph_ptr->data()->begin(); itr != graph_ptr->data()->end(); ++itr) {
        if (itr->key == frequency) {
            itr->value = value;
            break;
        }
    }
}

void ParametricEqView::SetBand(float frequency, float value) {
    auto graph_ptr = graph(dragable_graph_number_);
    for (auto itr = graph_ptr->data()->begin(); itr != graph_ptr->data()->end(); ++itr) {
        if (itr->key == frequency) {
            itr->value = value;
            break;
        }
    }
    replot();
}

void ParametricEqView::mousePressEvent(QMouseEvent* event) {
    double x, y;

    double best_dist = 1E+300;
    int best_index = 0;

    if ((dragable_graph_number_ >= 0) && (dragable_graph_number_ < this->graphCount())) {
	    const auto* pq_graph = graph(dragable_graph_number_);
        pq_graph->pixelsToCoords(event->localPos(), x, y);

        double nearest = -1;
        double nearest_diff = DBL_MAX;
        for (int i = 0; i < kEqDefaultFrequencies.size(); i++) {
            double diff = std::abs(x - kEqDefaultFrequencies[i]);
            if (diff < nearest_diff) {
                nearest_diff = diff;
                nearest = kEqDefaultFrequencies[i];
            }
        }
        x = nearest;

        if (pq_graph->data()->size() >= 1) {
            for (int n = 0; n < (pq_graph->data()->size()); n++) {
                double dist = fabs((pq_graph->data()->begin() + n)->value - y);
                dist += fabs((pq_graph->data()->begin() + n)->key - x);
                if (dist < best_dist) {
                    best_dist = dist;
                    best_index = n;
                }
            }
            drag_number_ = best_index;
        }
    }
}

void ParametricEqView::mouseReleaseEvent(QMouseEvent* event) {
    drag_number_ = -1;
    if ((dragable_graph_number_ >= 0) && (dragable_graph_number_ < graphCount())) {
        graph(dragable_graph_number_)->data().data()->sort();
    }
    replot();
    QCustomPlot::mouseReleaseEvent(event);
}

void ParametricEqView::mouseMoveEvent(QMouseEvent* event) {
    double x = 0, y = 0;

    if ((dragable_graph_number_ >= 0) && (dragable_graph_number_ < graphCount())) {
	    const auto* pq_graph = graph(dragable_graph_number_);
        pq_graph->pixelsToCoords(event->localPos(), x, y);

        double nearest = -1;
        double nearest_diff = DBL_MAX;
        for (int i = 0; i < kEqDefaultFrequencies.size(); i++) {
	        const double diff = std::abs(x - kEqDefaultFrequencies[i]);
            if (diff < nearest_diff) {
                nearest_diff = diff;
                nearest = kEqDefaultFrequencies[i];
            }
        }

        x = nearest;
        y = round(y * 16) / 16;

        if (y > kEQMaxDb)
            y = kEQMaxDb;
        else if (y < kEQMinDb)
            y = kEQMinDb;

        if (drag_number_ >= 0) {
            (pq_graph->data()->begin() + drag_number_)->value = y;
            emit DataChanged(x, y);
            replot();
        }
    }
}
