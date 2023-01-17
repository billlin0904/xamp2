#include <QFontDatabase>
#include <QPainter>
#include <QApplication>
#include <QIconEngine>
#include <QtCore>
#include <QPalette>

#include <utility>
#include <widget/str_utilts.h>
#include <widget/fonticonanimation.h>
#include <widget/fonticon.h>

class FontIconEngine : public QIconEngine {
public:
    explicit FontIconEngine(QVariantMap opt);

    void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) override;

    QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override;

    QIconEngine* clone() const override;

    void setFontFamily(const QString& family);

    void setLetter(const char32_t letter);
private:
    char32_t letter_;
    QString font_family_;
    QVariantMap options_;
};

const QString FontIconOption::animationAttr(qTEXT("animation"));
const QString FontIconOption::rectAttr(qTEXT("rect"));
const QString FontIconOption::scaleFactorAttr(qTEXT("scaleFactor"));
const QString FontIconOption::fontStyleAttr(qTEXT("fontStyle"));
const QString FontIconOption::colorAttr(qTEXT("color"));
const QString FontIconOption::onColorAttr(qTEXT("onColor"));
const QString FontIconOption::activeColorAttr(qTEXT("activeColor"));
const QString FontIconOption::activeOnColorAttr(qTEXT("activeOnColor"));
const QString FontIconOption::disabledColorAttr(qTEXT("disabledColor"));
const QString FontIconOption::selectedColorAttr(qTEXT("selectedColor"));
const QString FontIconOption::flipLeftRightAttr(qTEXT("flipLeftRight"));
const QString FontIconOption::rotateAngleAttr(qTEXT("rotateAngle"));
const QString FontIconOption::flipTopBottomAttr(qTEXT("flipTopBottom"));
const QString FontIconOption::opacityAttr(qTEXT("opacity"));

QColor FontIconOption::color = QApplication::palette().color(QPalette::Normal, QPalette::ButtonText);
QColor FontIconOption::disabledColor = QApplication::palette().color(QPalette::Disabled, QPalette::ButtonText);
QColor FontIconOption::selectedColor = QApplication::palette().color(QPalette::Active, QPalette::ButtonText);
QColor FontIconOption::onColor(QColor::Invalid);
QColor FontIconOption::activeColor(QColor::Invalid);
QColor FontIconOption::activeOnColor(QColor::Invalid);
double FontIconOption::opacity = 1.0;

template <typename T>
static T getOrDefault(QVariantMap const & opt, const QString & s, T defaultValue) {
	const auto var = opt.value(s);
    if (!var.isValid()) {
        return defaultValue;
    }
    else {
        return var.value<T>();
    }
}

FontIconEngine::FontIconEngine(QVariantMap opt)
    : QIconEngine()
	, options_(std::move(opt)){
}

void FontIconEngine::paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) {
    Q_UNUSED(state)

	auto paint_rect = rect;

    QFont font(font_family_);
    font.setStyleStrategy(QFont::PreferAntialias);

    auto var = options_.value(FontIconOption::animationAttr);
    if (auto* animation = var.value<FontIconAnimation*>()) {
        animation->setup(*painter, rect);
    }

    var = options_.value(FontIconOption::rectAttr);
    if (var.isValid()) {
        paint_rect = var.value<QRect>();
    }

    var = options_.value(FontIconOption::scaleFactorAttr);

    // A 16 pixel-high icon yields a font size of 14, which is pixel perfect
    // for font-awesome. 16 * 0.875 = 14
    // The reason why the glyph size is smaller than the icon size is to
	// account for font bearing.
    //constexpr auto kRatio = 0.875;
    constexpr auto kRatio = 0.875;
    int draw_size = qRound(paint_rect.height() * kRatio);
    if (var.isValid()) {
        draw_size = qRound(paint_rect.height() * kRatio * var.value<qreal>());
    }

    var = options_.value(FontIconOption::fontStyleAttr);
    if (var.isValid()) {
        font.setStyleName(var.value<QString>());
    }
    font.setPixelSize(draw_size);

    QColor pen_color;

    switch (mode) {
    case QIcon::Normal:
        switch (state) {
        case QIcon::On:
            pen_color = getOrDefault<QColor>(options_, FontIconOption::onColorAttr, FontIconOption::onColor);
            break;
        }
        break;
    case QIcon::Active:
        switch (state) {
		case QIcon::Off:
            pen_color = getOrDefault<QColor>(options_, FontIconOption::activeColorAttr, FontIconOption::activeColor);
			break;
		case QIcon::On:
            pen_color = getOrDefault<QColor>(options_, FontIconOption::activeOnColorAttr, FontIconOption::activeOnColor);
            if (!pen_color.isValid()) {
                pen_color = getOrDefault<QColor>(options_, FontIconOption::onColorAttr, FontIconOption::onColor);
            }
            break;
        }
        break;
    case QIcon::Disabled:
        pen_color = getOrDefault<QColor>(options_, FontIconOption::disabledColorAttr, FontIconOption::disabledColor);
        break;
    case QIcon::Selected:
        pen_color = getOrDefault<QColor>(options_, FontIconOption::selectedColorAttr, FontIconOption::selectedColor);
        break;
    }

    if (!pen_color.isValid()) {
        pen_color = getOrDefault<QColor>(options_, FontIconOption::colorAttr, FontIconOption::color);
    }

    painter->save();

    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter->translate(rect.center() + QPoint(1, 1));

    var = options_.value(FontIconOption::opacityAttr);
    if (var.isValid()) {
        painter->setOpacity(var.value<qreal>());
    } else {
        painter->setOpacity(FontIconOption::opacity);
    }

    var = options_.value(FontIconOption::rotateAngleAttr);
    if (var.isValid()) {
        painter->rotate(var.value<qreal>());
    }

    var = options_.value(FontIconOption::flipLeftRightAttr);
    if (var.isValid()) {
        painter->scale(-1.0, 1.0);
    }

    var = options_.value(FontIconOption::flipTopBottomAttr);
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

void FontIconEngine::setLetter(const char32_t letter) {
    letter_ = letter;
}

QIconEngine* FontIconEngine::clone() const {
    auto* engine = new FontIconEngine(options_);
    engine->setFontFamily(font_family_);
    return engine;
}

HashMap<char32_t, uint32_t> FontIcon::glyphs_;

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

QIcon FontIcon::animationIcon(const char32_t& code, QWidget* parent, const QString& family) const {
    QVariantMap options;
    options.insert(FontIconOption::animationAttr, QVariant::fromValue(new FontIconAnimation(parent)));
    return icon(code, options, family);
}

QIcon FontIcon::icon(const char32_t& code, QVariantMap options, const QString& family) const {
    if (families().isEmpty()) {
        return {};
    }

    QString use_family = family;
    if (use_family.isEmpty()) {
        use_family = families().first();
    }

    auto* engine = new FontIconEngine(std::move(options));
    engine->setFontFamily(use_family);
    if (!glyphs_.contains(code)) {
        engine->setLetter(code);
    }
    else {
        engine->setLetter(glyphs_[code]);
    }
    return QIcon(engine);
}

const QStringList& FontIcon::families() const {
    return families_;
}

void FontIcon::addFamily(const QString& family) {
    families_.append(family);
}

void FontIcon::setGlyphs(const HashMap<char32_t, uint32_t>& glyphs) {
    glyphs_ = glyphs;
}