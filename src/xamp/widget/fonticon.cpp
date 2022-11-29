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

QColor FontIconColorOption::color = QApplication::palette("QWidget").color(QPalette::Normal, QPalette::ButtonText);
QColor FontIconColorOption::disabledColor = QApplication::palette("QWidget").color(QPalette::Disabled, QPalette::ButtonText);
QColor FontIconColorOption::selectedColor = QApplication::palette("QWidget").color(QPalette::Active, QPalette::ButtonText);
QColor FontIconColorOption::onColor;
QColor FontIconColorOption::activeColor;
QColor FontIconColorOption::activeOnColor;

template <typename T>
T getOrDefault(QVariantMap const & opt, ConstLatin1String s, T defaultValue) {
	const auto var = opt.value(qTEXT("color"));
    if (!var.isValid()) {
        return defaultValue;
    }
    else {
        return var.value<QColor>();
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

    auto var = options_.value(qTEXT("animation"));
    if (auto* animation = var.value<FontIconAnimation*>()) {
        animation->setup(*painter, rect);
    }

    var = options_.value(qTEXT("rect"));
    if (var.isValid()) {
        paint_rect = var.value<QRect>();
    }

    var = options_.value(qTEXT("scaleFactor"));
    int draw_size = qRound(paint_rect.height() * 0.9);
    if (var.isValid()) {
        draw_size = qRound(paint_rect.height() * var.value<qreal>());
    }

    var = options_.value(qTEXT("fontStyle"));
    if (var.isValid()) {
        font.setStyleName(var.value<QString>());
    }

    font.setPixelSize(draw_size);

    auto pen_color = getOrDefault<QColor>(options_, qTEXT("color"), FontIconColorOption::color);

    switch (mode) {
    case QIcon::Normal:
        pen_color = getOrDefault<QColor>(options_, qTEXT("onColor"), FontIconColorOption::onColor);
        break;
    case QIcon::Active:
        switch (state) {
		case QIcon::Off:
            pen_color = getOrDefault<QColor>(options_, qTEXT("activeColor"), FontIconColorOption::activeColor);
			break;
		case QIcon::On:
            pen_color = getOrDefault<QColor>(options_, qTEXT("activeOnColor"), FontIconColorOption::activeColor);
            if (!pen_color.isValid()) {
                pen_color = getOrDefault<QColor>(options_, qTEXT("onColor"), FontIconColorOption::onColor);
            }
            break;
        }
        break;
    case QIcon::Disabled:
        pen_color = getOrDefault<QColor>(options_, qTEXT("disabledColor"), FontIconColorOption::disabledColor);
        break;
    case QIcon::Selected:
        pen_color = getOrDefault<QColor>(options_, qTEXT("selectedColor"), FontIconColorOption::selectedColor);
        break;
    }

    painter->save();

    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter->translate(rect.center() + QPoint(1, 1));

    var = options_.value(qTEXT("rotateAngle"));
    if (var.isValid()) {
        painter->rotate(var.value<qreal>());
    }

    var = options_.value(qTEXT("flipLeftRight"));
    if (var.isValid()) {
        painter->scale(-1.0, 1.0);
    }

    var = options_.value(qTEXT("flipTopBottom"));
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

//#define USE_SEGOE_FLUENT_ICON_FONT

HashMap<char32_t, uint32_t> FontIcon::glyphs_ = {
#ifdef USE_SEGOE_FLUENT_ICON_FONT
    { ICON_VOLUME_UP ,                0xE995},
    { ICON_VOLUME_OFF,                0xE74F},
    { ICON_SPEAKER,                   0xE7F5},
    { ICON_FOLDER,                    0xF12B},
    { ICON_AUDIO,                     0xE8D6},
    { ICON_LOAD_FILE,                 0xE8E5},
    { ICON_LOAD_DIR,                  0xE838},
    { ICON_RELOAD,                    0xE895},
    { ICON_REMOVE_ALL,                0xE894},
    { ICON_OPEN_FILE_PATH,            0xE8DA},
    { ICON_READ_REPLAY_GAIN,          0xF270},
    { ICON_EXPORT_FILE,               0xE78C},
    { ICON_COPY,                      0xE8C8},
    { ICON_DOWNLOAD,                  0xE896},
    { ICON_PLAYLIST,                  0xE90B},
    { ICON_EQUALIZER,                 0xE9E9},
    { ICON_PODCAST,                   0xEFA9},
    { ICON_ALBUM,                     0xE93C},
    { ICON_CD,                        0xE958},
    { ICON_LEFT_ARROW,                0xEC52},
    { ICON_ARTIST,                    0xE716},
    { ICON_SUBTITLE,                  0xED1E},
    { ICON_PREFERENCE,                0xE713},
    { ICON_ABOUT,                     0xF167},
    { ICON_DARK_MODE,                 0xEC46},
    { ICON_LIGHT_MODE,                0xF08C},
    { ICON_SEARCH,                    0xF78B},
    { ICON_THEME,                     0xE771},
    { ICON_DESKTOP,                   0xEC4E},
    { ICON_SHUFFLE_PLAY_ORDER,        0xE8B1},
    { ICON_REPEAT_ONE_PLAY_ORDER,     0xE8ED},
    { ICON_REPEAT_ONCE_PLAY_ORDER,    0xE8EE},
    { ICON_MINIMIZE_WINDOW,           0xE921},
    { ICON_MAXIMUM_WINDOW,            0xE922},
    { ICON_CLOSE_WINDOW,              0xE8BB},
    { ICON_RESTORE_WINDOW,            0xE923},
    { ICON_SLIDER_BAR,                0xE700},
    { ICON_PLAY,                      0xF5B0},
    { ICON_PAUSE,                     0xF8AE},
    { ICON_STOP_PLAY,                 0xE978},
    { ICON_PLAY_FORWARD,              0xF8AD},
    { ICON_PLAY_BACKWARD,             0xF8AC},
    { ICON_MORE,                      0xE712},
    { ICON_HIDE,                      0xED1A},
    { ICON_SHOW,                      0xE7B3},
    { ICON_USB,                       0xE88E},
    { ICON_BUILD_IN_SPEAKER,          0xE7F5},
    { ICON_BLUE_TOOTH,                0xE702},
#else
    { ICON_VOLUME_UP ,                0xF6A8},
    { ICON_VOLUME_OFF,                0xF6A9},
    { ICON_SPEAKER,                   0xF8DF},
    { ICON_FOLDER,                    0xF07B},
    { ICON_AUDIO,                     0xF001},
    { ICON_LOAD_FILE,                 0xF15B},
    { ICON_LOAD_DIR,                  0xF07C},
    { ICON_RELOAD,                    0xF2F9},
    { ICON_REMOVE_ALL,                0xE2AE},
    { ICON_OPEN_FILE_PATH,            0xF07C},
    { ICON_READ_REPLAY_GAIN,          0xF5F0},
    { ICON_EXPORT_FILE,               0xF56E},
    { ICON_COPY,                      0xF0C5},
    { ICON_DOWNLOAD,                  0xF0ED},
    { ICON_PLAYLIST,                  0xF8C9},
    { ICON_EQUALIZER,                 0xF3F2},
    { ICON_PODCAST,                   0xF2CE},
    { ICON_ALBUM,                     0xF89F},
    { ICON_CD,                        0xF51F},
    { ICON_LEFT_ARROW,                0xF177},
    { ICON_ARTIST,                    0xF500},
    { ICON_SUBTITLE,                  0xE1DE},
    { ICON_PREFERENCE,                0xF013},
    { ICON_ABOUT,                     0xF05A},
    { ICON_DARK_MODE,                 0xF186},
    { ICON_LIGHT_MODE,                0xF185},
    { ICON_SEARCH,                    0xF002},
    { ICON_THEME,                     0xF042},
    { ICON_DESKTOP,                   0xF390},
    { ICON_SHUFFLE_PLAY_ORDER,        0xF074},
    { ICON_REPEAT_ONE_PLAY_ORDER,     0xF363},
    { ICON_REPEAT_ONCE_PLAY_ORDER,    0xF365},
    { ICON_MINIMIZE_WINDOW,           0xF2D1},
    { ICON_MAXIMUM_WINDOW,            0xF2D0},
    { ICON_CLOSE_WINDOW,              0xF00D},
    { ICON_RESTORE_WINDOW,            0xE923},
    { ICON_SLIDER_BAR,                0xF0C9},
    { ICON_PLAY,                      0xF04B},
    { ICON_PAUSE,                     0xF04C},
    { ICON_STOP_PLAY,                 0xF04D},
    { ICON_PLAY_FORWARD,              0xF04E},
    { ICON_PLAY_BACKWARD,             0xF04A},
    { ICON_MORE,                      0xF142},
    { ICON_HIDE,                      0xF070},
    { ICON_SHOW,                      0xF06E},
    { ICON_USB,                       0xF8E9},
    { ICON_BUILD_IN_SPEAKER,          0xF8DF},
    { ICON_BLUE_TOOTH,                0xF293},
#endif
};

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

QIcon FontIcon::animationIcon(const char32_t& code, QWidget* parent, const QString& family) const {
    QVariantMap options;
    options.insert(qTEXT("animation"), QVariant::fromValue(new FontIconAnimation(parent)));
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
    engine->setLetter(glyphs_[code]);
    return QIcon(engine);
}

const QStringList& FontIcon::families() const {
    return families_;
}

void FontIcon::addFamily(const QString& family) {
    families_.append(family);
}