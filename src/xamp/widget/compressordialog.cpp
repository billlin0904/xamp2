#include <widget/str_utilts.h>
#include <widget/compressordialog.h>

#include "appsettings.h"

CompressorDialog::CompressorDialog(QWidget* parent)
	: QDialog(parent) {
	ui_.setupUi(this);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	auto gain = AppSettings::getValue(kCompressorGain).toInt();
	auto threshold = AppSettings::getValue(kCompressorThreshold).toInt();
	auto ratio = AppSettings::getValue(kCompressorRatio).toInt();
	auto attack = AppSettings::getValue(kCompressorAttack).toInt();
	auto release = AppSettings::getValue(kCompressorRelease).toInt();

	ui_.gainSlider->setValue(gain);
	ui_.thresholdSlider->setValue(threshold);
	ui_.ratioSlider->setValue(ratio);
	ui_.attackTimeSlider->setValue(attack);

	(void)QObject::connect(ui_.gainSlider, &QSlider::valueChanged, [this](auto value) {
		ui_.gainLabel->setText(QString(Q_UTF8("%0 db")).arg(value));
		notifyChange();
		});

	(void)QObject::connect(ui_.thresholdSlider, &QSlider::valueChanged, [this](auto value) {
		ui_.thresholdLabel->setText(QString(Q_UTF8("%0 db")).arg(value));
		notifyChange();
		});

	(void)QObject::connect(ui_.ratioSlider, &QSlider::valueChanged, [this](auto value) {
		ui_.ratioLabel->setText(QString(Q_UTF8("1/%0")).arg(value));
		notifyChange();
		});

	(void)QObject::connect(ui_.attackTimeSlider, &QSlider::valueChanged, [this](auto value) {
		ui_.attackTimeLabel->setText(QString(Q_UTF8("%0 ms")).arg(value));
		notifyChange();
		});

	(void)QObject::connect(ui_.releaseTimeSlider, &QSlider::valueChanged, [this](auto value) {
		ui_.releaseTimeLabel->setText(QString(Q_UTF8("%0 ms")).arg(value));
		notifyChange();
		});
}

void CompressorDialog::notifyChange() {
	emit onValueChange(ui_.gainSlider->value(),
		ui_.thresholdSlider->value(),
		ui_.ratioSlider->value(),
		ui_.attackTimeSlider->value(),
		ui_.releaseTimeSlider->value());
}
