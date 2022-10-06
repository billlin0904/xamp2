#include <QPainter>
#include <QPaintEvent>

#include <player/audio_util.h>
#include <widget/waveform.h>

Waveform::Waveform(QWidget* parent)
	: QWidget(parent)
    , max_peak_(0) {
}

void Waveform::paintEvent(QPaintEvent* event) {
    establishDrawingMode();
    overviewDraw(event);
}

void Waveform::resizeEvent(QResizeEvent* event) {
}

void Waveform::load(const std::wstring& file_ext) {
    stream_ = MakeFileStream(file_ext);
    recalculatePeaks();
}

void Waveform::establishDrawingMode() {   
    if (size() != lastSize_) {
        recalculatePeaks();
    }
    lastSize_ = size();
}

void Waveform::overviewDraw(QPaintEvent* event) {
	QPainter painter(this);

	painter.setPen(QPen(waveformColor_, 1, Qt::SolidLine, Qt::RoundCap));

	auto minX = event->region().boundingRect().x();
	auto maxX = event->region().boundingRect().x() + event->region().boundingRect().width();

    auto startIndex = 2 * minX;
    auto endIndex = 2 * maxX;

    auto yMidpoint = this->height() / 2;
    auto counter = minX;

    for (int i = startIndex; i < endIndex; i += 2) {
        auto chan1YMidpoint = yMidpoint - height() / 4;
        auto chan2YMidpoint = yMidpoint + height() / 4;

        painter.drawLine(counter, chan1YMidpoint, counter, chan1YMidpoint + ((height() / 4) * peeks_.at(i) * scaleFactor_));
        painter.drawLine(counter, chan1YMidpoint, counter, chan1YMidpoint - ((height() / 4) * peeks_.at(i) * scaleFactor_));

        painter.drawLine(counter, chan2YMidpoint, counter, chan2YMidpoint + ((height() / 4) * peeks_.at(i + 1) * scaleFactor_));
        painter.drawLine(counter, chan2YMidpoint, counter, chan2YMidpoint - ((height() / 4) * peeks_.at(i + 1) * scaleFactor_));

        counter++;
    }
}

void Waveform::recalculatePeaks() {
	float peak = Max(GetNormalizedPeaks(stream_));

	scaleFactor_ = 1.0f / peak;
	scaleFactor_ = scaleFactor_ - scaleFactor_ * padding_;

	auto totalFrames = stream_->GetTotalFrames();
	auto frameIncrement = totalFrames / width();

    peeks_.clear();    
    peeks_.reserve(totalFrames);

    for (int i = 0; i < totalFrames; i += frameIncrement) {
        auto regionMax = peakForRegion(i, i + frameIncrement);
        double frameAbsL = fabs(regionMax[0]);
        double frameAbsR = fabs(regionMax[1]);

        peeks_.push_back(frameAbsL);
        peeks_.push_back(frameAbsR);
    }
}

std::vector<float> Waveform::peakForRegion(int region_start_frame, int region_end_frame) {
    std::vector<float> regionPeak;
    std::vector<float> chunk(2 * (region_end_frame - region_start_frame));
    stream_->Seek(region_start_frame);

    if (stream_->GetSamples(chunk.data(), chunk.size()) == 0) {
        return regionPeak;
    }

    float max0 = 0.0f;
    float max1 = 0.0f;

    for (int i = 0; i < 2 * (region_end_frame - region_start_frame); i += 2) {        
        max0 = (std::max)(fabs(chunk[i]), fabs(max0));
        max1 = (std::max)(fabs(chunk[i + 1]), fabs(max1));
    }

    regionPeak.push_back(max0);
    regionPeak.push_back(max1);
    return regionPeak;
}
