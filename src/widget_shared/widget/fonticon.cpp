#include <widget/fonticon.h>

#include <QFontDatabase>
#include <QPainter>
#include <QApplication>
#include <QIconEngine>
#include <QtCore>
#include <QPalette>

#include <widget/util/str_util.h>
#include <widget/widget_shared.h>
#include <utility>


class XAMP_WIDGET_SHARED_EXPORT FontIconEngine : public QIconEngine {
public:
    explicit FontIconEngine(QVariantMap opt);

    void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) override;

    QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override;

    QIconEngine* clone() const override;

    void setFontFamily(const QString& family);

    void setLetter(const int32_t letter);
private:
    int32_t letter_;
    QString font_family_;
    QVariantMap options_;
};

const QString FontIconOption::kRectAttr(qTEXT("rect"));
const QString FontIconOption::kScaleFactorAttr(qTEXT("scaleFactor"));
const QString FontIconOption::kFontStyleAttr(qTEXT("fontStyle"));
const QString FontIconOption::kColorAttr(qTEXT("color"));
const QString FontIconOption::kOnColorAttr(qTEXT("onColor"));
const QString FontIconOption::kActiveColorAttr(qTEXT("activeColor"));
const QString FontIconOption::kActiveOnColorAttr(qTEXT("activeOnColor"));
const QString FontIconOption::kDisabledColorAttr(qTEXT("disabledColor"));
const QString FontIconOption::kSelectedColorAttr(qTEXT("selectedColor"));
const QString FontIconOption::kFlipLeftRightAttr(qTEXT("flipLeftRight"));
const QString FontIconOption::kRotateAngleAttr(qTEXT("rotateAngle"));
const QString FontIconOption::kFlipTopBottomAttr(qTEXT("flipTopBottom"));
const QString FontIconOption::kOpacityAttr(qTEXT("opacity"));
const QString FontIconOption::kAnimation(qTEXT("animation"));

QColor FontIconOption::color = QApplication::palette().color(QPalette::Normal, QPalette::ButtonText);
QColor FontIconOption::disabledColor = QApplication::palette().color(QPalette::Disabled, QPalette::ButtonText);
QColor FontIconOption::selectedColor = QApplication::palette().color(QPalette::Active, QPalette::ButtonText);
QColor FontIconOption::onColor(QColor::Invalid);
QColor FontIconOption::activeColor(QColor::Invalid);
QColor FontIconOption::activeOnColor(QColor::Invalid);
double FontIconOption::opacity = 1.0;

namespace {
    template <typename T>
    T GetOrDefault(QVariantMap const& opt, const QString& s, T defaultValue) {
        const auto var = opt.value(s);
        if (!var.isValid()) {
            return defaultValue;
        }
        else {
            return var.value<T>();
        }
    }
}

FontIconEngine::FontIconEngine(QVariantMap opt)
    : QIconEngine()
    , letter_(0)
	, options_(std::move(opt)){
}

void FontIconEngine::paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) {
    Q_UNUSED(state)

	auto paint_rect = rect;

    QFont font(font_family_);
    font.setStyleStrategy(QFont::PreferAntialias);

    auto var = options_.value(FontIconOption::kRectAttr);
    if (var.isValid()) {
        paint_rect = var.value<QRect>();
    }

    var = options_.value(FontIconOption::kScaleFactorAttr);

    constexpr auto kRatio = 1.0;
    int draw_size = qRound(paint_rect.height() * kRatio);
    if (var.isValid()) {
        draw_size = qRound(paint_rect.height() * kRatio * var.value<qreal>());
    }

    var = options_.value(FontIconOption::kFontStyleAttr);
    if (var.isValid()) {
        font.setStyleName(var.value<QString>());
    }
    font.setPixelSize(draw_size);    

    QColor pen_color;

    switch (mode) {
    case QIcon::Normal:
        switch (state) {
        case QIcon::On:
            pen_color = GetOrDefault<QColor>(options_, FontIconOption::kOnColorAttr, FontIconOption::onColor);
            break;
        }
        break;
    case QIcon::Active:
        switch (state) {
		case QIcon::Off:
            pen_color = GetOrDefault<QColor>(options_, FontIconOption::kActiveColorAttr, FontIconOption::activeColor);
			break;
		case QIcon::On:
            pen_color = GetOrDefault<QColor>(options_, FontIconOption::kActiveOnColorAttr, FontIconOption::activeOnColor);
            if (!pen_color.isValid()) {
                pen_color = GetOrDefault<QColor>(options_, FontIconOption::kOnColorAttr, FontIconOption::onColor);
            }
            break;
        }
        break;
    case QIcon::Disabled:
        pen_color = GetOrDefault<QColor>(options_, FontIconOption::kDisabledColorAttr, FontIconOption::disabledColor);
        break;
    case QIcon::Selected:
        pen_color = GetOrDefault<QColor>(options_, FontIconOption::kSelectedColorAttr, FontIconOption::selectedColor);
        break;
    }

    if (!pen_color.isValid()) {
        pen_color = GetOrDefault<QColor>(options_, FontIconOption::kColorAttr, FontIconOption::color);
    }

    painter->save();

    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter->translate(rect.center() + QPoint(1, 1));

    var = options_.value(FontIconOption::kOpacityAttr);
    if (var.isValid()) {
        painter->setOpacity(var.value<qreal>());
    } else {
        painter->setOpacity(FontIconOption::opacity);
    }

    var = options_.value(FontIconOption::kRotateAngleAttr);
    if (var.isValid()) {
        painter->rotate(var.value<qreal>());
    }

    var = options_.value(FontIconOption::kFlipLeftRightAttr);
    if (var.isValid()) {
        painter->scale(-1.0, 1.0);
    }

    var = options_.value(FontIconOption::kFlipTopBottomAttr);
    if (var.isValid()) {
        painter->scale(1.0, -1.0);
    }
    painter->translate(-rect.center() - QPoint(1, 1));
    painter->setPen(QPen(pen_color));
    painter->setFont(font);
    painter->drawText(paint_rect, Qt::AlignCenter, QChar(letter_));
    painter->restore();
}

QPixmap FontIconEngine::pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) {
    QPixmap pix(size);
    pix.fill(Qt::transparent);

    QPainter painter(&pix);
    paint(&painter, QRect(QPoint(0, 0), size), mode, state);
    return pix;
}

void FontIconEngine::setFontFamily(const QString& family) {
    font_family_ = family;
}

void FontIconEngine::setLetter(const int32_t letter) {
    letter_ = letter;
}

QIconEngine* FontIconEngine::clone() const {
    auto* engine = new FontIconEngine(options_);
    engine->setFontFamily(font_family_);
    return engine;
}

FontIcon::FontIcon(QObject* parent)
    : QObject(parent) {
}

bool FontIcon::addFont(const QString& filename) {
    QFile font_file(filename);
    if (!font_file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QByteArray font_data(font_file.readAll());
    font_file.close();
    const auto id = QFontDatabase::addApplicationFontFromData(font_data);
    if (id == -1) {
        return false;
    }

    const auto family = QFontDatabase::applicationFontFamilies(id).first();
    addFamily(family);
    return true;
}

QIcon FontIcon::getIcon(const int32_t& code, QVariantMap options, const QString& family) const {
    if (getFamilies().isEmpty()) {
        return {};
    }

    QString use_family = family;
    if (use_family.isEmpty()) {
        use_family = getFamilies().first();
    }

    auto* engine = new FontIconEngine(std::move(options));
    engine->setFontFamily(use_family);
    if (glyphs_.find(code) == glyphs_.end()) {
        engine->setLetter(code);
    }
    else {
        engine->setLetter(glyphs_[code]);
    }
    return QIcon(engine);
}

const QStringList& FontIcon::getFamilies() const {
    return families_;
}

void FontIcon::addFamily(const QString& family) {
    families_.append(family);
}

void FontIcon::setGlyphs(const HashMap<int32_t, uint32_t>& glyphs) {
    glyphs_ = glyphs;
}
