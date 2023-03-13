#include <array>
#include <widget/str_utilts.h>
#include <widget/parametriceqview.h>

#include "thememanager.h"

inline constexpr std::array<double, 11> kBands = {
	0, 32, 64, 125, 250, 500, 1000, 2000, 4000, 8000, 16000
};

inline constexpr std::array<double, 11> kDBs = {
    -15, -12, -9, -6, -3, 0, 3, 6, 9, 12, 15
};

const QStringList kBandsStr = QStringList()
<< qTEXT("0")
<< qTEXT("32")
<< qTEXT("64")
<< qTEXT("125")
<< qTEXT("250")
<< qTEXT("500")
<< qTEXT("1K")
<< qTEXT("2K")
<< qTEXT("4k")
<< qTEXT("8k")
<< qTEXT("16k");

const QStringList kDBStr = QStringList()
<< qTEXT("-15dB")
<< qTEXT("-12dB")
<< qTEXT("-9dB")
<< qTEXT("-6dB")
<< qTEXT("-3dB")
<< qTEXT("0dB")
<< qTEXT("+3dB")
<< qTEXT("+6dB")
<< qTEXT("+9dB")
<< qTEXT("+12dB")
<< qTEXT("+15dB");


ParametricEqView::ParametricEqView(QWidget* parent) {
    QFont f(qTEXT("FormatFont"));
    f.setWeight(QFont::Light);
    f.setPointSize(qTheme.GetFontSize(5));
    setOpenGl(true);

    xAxis->setScaleType(QCPAxis::stLogarithmic);
    setInteraction(QCP::Interaction::iRangeDrag, false);
    setInteraction(QCP::Interaction::iRangeZoom, false);
    yAxis->setRange(QCPRange(min_db, max_db));
    xAxis->setRange(QCPRange(25, 16000));

    yAxis->setLabelFont(f);
    xAxis->setLabelFont(f);

    addGraph();
    addGraph();

    graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QPen(Qt::black, 1.5), QBrush(Qt::white), 9));
    graph(0)->setPen(QPen(QColor(42, 130, 218), 4));

    for (auto i = 0; i < kBands.size(); i++)
        graph(0)->addData(kBands[i], 0);

    const QSharedPointer<QCPAxisTickerText> fixed_ticker(new QCPAxisTickerText());
    xAxis->setTicker(fixed_ticker);
    fixed_ticker->setTickCount(kBands.size());
    for (auto i = 0; i < kBands.size(); i++)
        fixed_ticker->addTick(kBands[i], kBandsStr.at(i));

    for (auto i = 0; i < kDBs.size(); i++)
        graph(0)->addData(kDBs[i], 0);

    const QSharedPointer<QCPAxisTickerText> y_fixed_ticker(new QCPAxisTickerText());
    yAxis->setTicker(y_fixed_ticker);
    y_fixed_ticker->setTickCount(kDBs.size());
    for (auto i = 0; i < kDBs.size(); i++)
        y_fixed_ticker->addTick(kDBs[i], kDBStr.at(i));

    xAxis->setBasePen(QPen(qTheme.GetThemeTextColor(), 1));
    yAxis->setBasePen(QPen(qTheme.GetThemeTextColor(), 1));
    xAxis->setTickPen(QPen(qTheme.GetThemeTextColor(), 1));
    yAxis->setTickPen(QPen(qTheme.GetThemeTextColor(), 1));
    xAxis->setSubTickPen(QPen(QColor(140, 140, 140), 1));
    yAxis->setSubTickPen(QPen(QColor(140, 140, 140), 1));
    xAxis->setTickLabelColor(QColor(140, 140, 140));
    yAxis->setTickLabelColor(QColor(140, 140, 140));
    xAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    yAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    xAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    yAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
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
    setBackground(qTheme.BackgroundColor());

    resize(720, 300);
    setMinimumSize(QSize(720, 300));
    setMaximumSize(QSize(720, 300));
}

void ParametricEqView::mousePressEvent(QMouseEvent* event) {
    double x, y;

    double best_dist = 1E+300;
    int best_index = 0;

    if ((dragable_graph_number >= 0) && (dragable_graph_number < this->graphCount())) {
	    const auto* pq_graph = graph(dragable_graph_number);
        pq_graph->pixelsToCoords(event->localPos(), x, y);

        double nearest = -1;
        double nearest_diff = DBL_MAX;
        for (int i = 0; i < kBands.size(); i++) {
            double diff = std::abs(x - kBands[i]);
            if (diff < nearest_diff) {
                nearest_diff = diff;
                nearest = kBands[i];
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
            drag_number = best_index;
        }
    }
}

void ParametricEqView::mouseReleaseEvent(QMouseEvent* event) {
    drag_number = -1;
    if ((dragable_graph_number >= 0) && (dragable_graph_number < graphCount())) {
        graph(dragable_graph_number)->data().data()->sort();
    }
    replot();
    QCustomPlot::mouseReleaseEvent(event);
}

void ParametricEqView::mouseMoveEvent(QMouseEvent* event) {
    double x = 0, y = 0;

    if ((dragable_graph_number >= 0) && (dragable_graph_number < graphCount())) {
	    const auto* pq_graph = graph(dragable_graph_number);
        pq_graph->pixelsToCoords(event->localPos(), x, y);

        double nearest = -1;
        double nearest_diff = DBL_MAX;
        for (int i = 0; i < kBands.size(); i++) {
	        const double diff = std::abs(x - kBands[i]);
            if (diff < nearest_diff) {
                nearest_diff = diff;
                nearest = kBands[i];
            }
        }

        x = nearest;
        y = round(y * 16) / 16;

        if (y > max_db)
            y = max_db;
        else if (y < min_db)
            y = min_db;

        if (drag_number >= 0) {
            (pq_graph->data()->begin() + drag_number)->value = y;
            replot();
        }
    }
}
