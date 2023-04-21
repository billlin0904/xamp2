#include <widget/switchbutton.h>

#include <QStyleOptionButton>
#include <QPainter>
#include <QVariantAnimation>

#include <thememanager.h>
#include <widget/str_utilts.h>

SwitchButton::SwitchButton(QWidget *parent)
    : QPushButton(parent) {

    QString handle_color;

    is_dark_theme_ = qTheme.GetThemeColor() == ThemeColor::DARK_THEME;
    if (is_dark_theme_) {
        handle_color = qTEXT("black");
    } else {
        handle_color = qTEXT("white");
    }

    setStyleSheet(qSTR(R"(
    SwitchButton {		
        background: #CCCCCC;
        border: 1px solid darkgray;
        border-radius: 12px;
        height: 24px;
    }

    SwitchButton::handle {
        background: %1;
        border: none;
		max-height: 20px;
		min-width: 20px;
        max-width: 20px;
        border-radius: 9px;
    }

    SwitchButton:on {
        background: rgb(42, 130, 218);
    }
    )").arg(handle_color));

}

void SwitchButton::SetSwitchOn(bool checked) {
    checked_ = checked;
    progress_ = checked ? 1.0 : 0.0;
    update();
}

void SwitchButton::nextCheckState() {
    QAbstractButton::nextCheckState();

    auto *animation = new QVariantAnimation(this);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setDuration(200);
    (void)QObject::connect(animation,
                     &QVariantAnimation::valueChanged, this, [this](const QVariant & val){
        progress_ = val.toReal();
        update();
    });
    animation->setEasingCurve(QEasingCurve::OutQuad);

    const auto checked = !checked_;

    const auto direction =
        checked ? QAbstractAnimation::Forward : QAbstractAnimation::Backward;
    const auto pause = animation->state() == QAbstractAnimation::Running;
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

    QStyleOptionButton button_opt;
    initStyleOption(&button_opt);
    button_opt.rect.adjust(0, 0, -1, 0);
    button_opt.state &= ~QStyle::State_On;
    style()->drawControl(QStyle::CE_PushButtonBevel, &button_opt, &painter, this);
    painter.setOpacity(progress_);
    button_opt.state |= QStyle::State_On;
    style()->drawControl(QStyle::CE_PushButtonBevel, &button_opt, &painter, this);

    painter.setOpacity(1.0);

    const auto text_y = qMin(height(), width());
    painter.setPen(QPen(Qt::white));
    auto f = font();
    f.setBold(true);
    painter.setFont(f);

    if (checked_) {
        QString handle_color;

        if (is_dark_theme_) {
            handle_color = qTEXT("black");
        }
        else {
            handle_color = qTEXT("white");
        }

        QRect text_rect(0, 0, width() - text_y, height());
        painter.drawText(text_rect, Qt::AlignCenter, tr("ON"));

        setStyleSheet(qSTR(R"(
        SwitchButton {		
        background: #CCCCCC;
        border: 1px solid darkgray;
        border-radius: 12px;
        height: 24px;
        }

        SwitchButton::handle {
            background: %1;
            border: none;
		    max-height: 20px;
		    min-width: 20px;
            max-width: 20px;
            border-radius: 9px;
        }

        SwitchButton:on {
            background: rgb(42, 130, 218);
        }
        )").arg(handle_color));
    } else {
        QRect text_rect(0, 0, width() + text_y, height());
        painter.drawText(text_rect, Qt::AlignCenter, tr("OFF"));

        setStyleSheet(qSTR(R"(
        SwitchButton {		
        background: #CCCCCC;
        border: 1px solid darkgray;
        border-radius: 12px;
        height: 24px;
        }

        SwitchButton::handle {
            background: %1;
            border: none;
		    max-height: 20px;
		    min-width: 20px;
            max-width: 20px;
            border-radius: 9px;
        }

        SwitchButton:on {
            background: rgb(42, 130, 218);
        }
        )").arg(qTEXT("gray")));
    }

    QStyleOptionSlider slider_opt;
    slider_opt.init(this);
    slider_opt.minimum = 0;
    slider_opt.maximum = slider_opt.rect.width();
    const auto position = static_cast<int>(progress_ * (slider_opt.rect.width()));
    slider_opt.sliderPosition = qMin(qMax(position, 0), slider_opt.maximum);
    slider_opt.sliderValue = slider_opt.sliderPosition;
    slider_opt.rect = style()->subControlRect(QStyle::CC_ScrollBar, &slider_opt, QStyle::SC_ScrollBarSlider, this);
    style()->drawControl(QStyle::CE_ScrollBarSlider, &slider_opt, &painter, this);
}
