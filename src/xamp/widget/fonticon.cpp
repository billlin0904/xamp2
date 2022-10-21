#include <QFontDatabase>
#include <widget/str_utilts.h>
#include <widget/fonticonanimation.h>
#include <widget/fonticon.h>

FontIconEngine::FontIconEngine(QVariantMap opt)
    : QIconEngine()
	, selected_state_(false)
	, options_(std::move(opt)){
}

void FontIconEngine::paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) {
    Q_UNUSED(state)

    QFont font(font_family_);
	int draw_size = qRound(rect.height() * 0.9);
    font.setPixelSize(draw_size);
    font.setStyleStrategy(QFont::PreferAntialias);

    auto var = options_.value(Q_TEXT("animation"));
    if (auto* animation = var.value<FontIconAnimation*>()) {
        animation->setup(*painter, rect);
    }

    painter->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    QColor pen_color;
    if (!base_color_.isValid()) {
        pen_color = QApplication::palette("QWidget").color(QPalette::Normal, QPalette::ButtonText);
    } else {
        pen_color = base_color_;
    }

    if (mode == QIcon::Disabled) {
        pen_color = QApplication::palette("QWidget").color(QPalette::Disabled, QPalette::ButtonText);
    }
    if (selected_state_) {
        if (mode == QIcon::Selected) {
            pen_color = QApplication::palette("QWidget").color(QPalette::Active, QPalette::ButtonText);
        }
    }
    painter->save();
    painter->setPen(QPen(pen_color));
    painter->setFont(font);
    painter->drawText(rect, Qt::AlignCenter | Qt::AlignVCenter, letter_);
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

void FontIconEngine::setLetter(const QChar& letter) {
    letter_ = letter;
}

void FontIconEngine::setBaseColor(const QColor& base_color) {
    base_color_ = base_color;
}

void FontIconEngine::setSelectedState(bool enable) {
    selected_state_ = enable;
}

QIconEngine* FontIconEngine::clone() const {
    auto* engine = new FontIconEngine(options_);
    engine->setFontFamily(font_family_);
    engine->setBaseColor(base_color_);
    engine->setSelectedState(selected_state_);
    return engine;
}

FontIcon::FontIcon(QObject* parent)
    : QObject(parent) {
}

bool FontIcon::addFont(const QString& filename) {
	const auto id = QFontDatabase::addApplicationFont(filename);
    if (id == -1) {
        return false;
    }

    const auto family = QFontDatabase::applicationFontFamilies(id).first();
    addFamily(family);
    return true;
}

QIcon FontIcon::animationIcon(const QChar& code, QWidget* parent, const QColor* color, const QString& family) const {
    QVariantMap options;
    options.insert(Q_TEXT("animation"), QVariant::fromValue(new FontIconAnimation(parent)));
    return icon(code, options, color, family);
}

QIcon FontIcon::icon(const QChar& code, QVariantMap options, const QColor* color, const QString& family) const {
    if (families().isEmpty()) {
        return {};
    }

    QString use_family = family;
    if (use_family.isEmpty()) {
        use_family = families().first();
    }

    auto* engine = new FontIconEngine(options);
    engine->setFontFamily(use_family);
    engine->setLetter(code);
    if (!color) {
        engine->setBaseColor(base_color_);
    } else {
        engine->setBaseColor(*color);
    }
    return QIcon(engine);
}

const QStringList& FontIcon::families() const {
    return families_;
}

void FontIcon::addFamily(const QString& family) {
    families_.append(family);
}

void FontIcon::setBaseColor(const QColor& base_color) {
    base_color_ = base_color;
}