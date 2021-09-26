#include <QStyleOptionButton>
#include <QPainter>
#include <QVariantAnimation>
#include <widget/str_utilts.h>
#include <widget/switchbutton.h>

SwitchButton::SwitchButton(QWidget *parent)
    : QPushButton(parent) {
    setStyleSheet(Q_UTF8(R"(
    SwitchButton {
        background: #CCCCCC;
        border: none;
        border-radius: 15px;
        height: 30px;
    }

    SwitchButton::handle {
        background: white;
        border: none;
        min-width: 30px;
        max-width: 30px;
        border-radius: 15px;
    }

    SwitchButton:on {
        background: rgb(42, 130, 218);
    }
    )"));
}

void SwitchButton::nextCheckState() {
    QAbstractButton::nextCheckState();

    auto *animation = new QVariantAnimation(this);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setDuration(200);
    QObject::connect(animation,
                     &QVariantAnimation::valueChanged, this, [this](const QVariant & val){
        progress_ = val.toReal();
        update();
    });
    animation->setEasingCurve(QEasingCurve::OutQuad);

    auto checked = !checked_;

    auto direction =
        checked ? QAbstractAnimation::Forward : QAbstractAnimation::Backward;
    bool pause = animation->state() == QAbstractAnimation::Running;
    if(pause)
        animation->pause();
    animation->setDirection(direction);
    if(pause)
        animation->resume();
    else
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    checked_ = checked;
    update();
}

void SwitchButton::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    QStyleOptionButton buttonOpt;
    initStyleOption(&buttonOpt);
    buttonOpt.rect.adjust(0, 0, -1, 0);
    buttonOpt.state &= ~QStyle::State_On;
    style()->drawControl(QStyle::CE_PushButtonBevel, &buttonOpt, &painter, this);
    //painter.setOpacity(progress_);
    buttonOpt.state |= QStyle::State_On;
    style()->drawControl(QStyle::CE_PushButtonBevel, &buttonOpt, &painter, this);

    int text_y = qMin(height(), width());
    painter.setPen(QPen(Qt::white));
    if (checked_) {
        QRect text_rect(0, 0, width() - text_y, height());
        painter.drawText(text_rect, Qt::AlignCenter, Q_UTF8("ON"));
    } else {
        QRect text_rect(0, 0, width() + text_y, height());
        painter.drawText(text_rect, Qt::AlignCenter, Q_UTF8("OFF"));
    }

    QStyleOptionSlider sliderOpt;
    sliderOpt.init(this);
    sliderOpt.minimum = 0;
    sliderOpt.maximum = sliderOpt.rect.width();
    int position = int(progress_ * (sliderOpt.rect.width()));
    sliderOpt.sliderPosition = qMin(qMax(position, 0), sliderOpt.maximum);
    sliderOpt.sliderValue = sliderOpt.sliderPosition;
    sliderOpt.rect = style()->subControlRect(QStyle::CC_ScrollBar, &sliderOpt, QStyle::SC_ScrollBarSlider, this);
    style()->drawControl(QStyle::CE_ScrollBarSlider, &sliderOpt, &painter, this);
}
