#include <thememanager.h>

#include <widget/util/image_util.h>
#include <widget/util/json_util.h>

#include <widget/appsettingnames.h>
#include <widget/widget_shared.h>
#include <widget/util/str_util.h>
#include <widget/appsettings.h>
#include <widget/iconsizestyle.h>
#include <widget/imagecache.h>

#include <QOperatingSystemVersion>
#include <QDirIterator>
#include <QGraphicsDropShadowEffect>
#include <QScreen>
#include <QResource>
#include <QApplication>
#include <QFontDatabase>
#include <QTextStream>
#include <QFileInfo>
#include <QToolButton>
#include <QLineEdit>
#include <QListView>
#include <QComboBox>

namespace {
    template <typename Iterator>
    void sortFontWeight(Iterator begin, Iterator end) {
        std::sort(begin, end,
            [](const auto& left_font_name, const auto& right_font_name) {
                auto getFontWeight = [](auto name) {
                    if (name.contains("Thin"_str, Qt::CaseInsensitive)) {
                        return 100 * 10;
                    }
                    if (name.contains("Hairline"_str, Qt::CaseInsensitive)) {
                        return 100 * 10;
                    }
                    if (name.contains("ExtraLight"_str, Qt::CaseInsensitive)) {
                        return 200 * 10;
                    }
                    if (name.contains("Light"_str, Qt::CaseInsensitive)) {
                        return 300 * 10;
                    }
                    if (name.contains("Normal"_str, Qt::CaseInsensitive)) {
                        return 400;
                    }
                    if (name.contains("Regular"_str, Qt::CaseInsensitive)) {
                        return 400;
                    }
                    if (name.contains("Medium"_str, Qt::CaseInsensitive)) {
                        return 500;
                    }
                    if (name.contains("SemiBold"_str, Qt::CaseInsensitive)) {
                        return 600;
                    }
                    if (name.contains("DemiBold"_str, Qt::CaseInsensitive)) {
                        return 600;
                    }
                    if (name.contains("Bold"_str, Qt::CaseInsensitive)) {
                        return 700;
                    }
                    if (name.contains("ExtraBold"_str, Qt::CaseInsensitive)) {
                        return 800;
                    }
                    if (name.contains("UltraBold"_str, Qt::CaseInsensitive)) {
                        return 800;
                    }
                    if (name.contains("Black"_str, Qt::CaseInsensitive)) {
                        return 900;
                    }
                    if (name.contains("Heavy"_str, Qt::CaseInsensitive)) {
                        return 900;
                    }
                    return 100;
                    };
                return getFontWeight(left_font_name) < getFontWeight(right_font_name);
            });
    }
}

QString ThemeManager::countryFlagFilePath(const QString& country_iso_code) {
    return
        qFormat("%1/flags/%2.png")
        .arg(QCoreApplication::applicationDirPath())
        .arg(country_iso_code);
}

QString ThemeManager::fontNamePath(const QString& file_name) {
	return
		qFormat("%1/fonts/%2")
		.arg(QCoreApplication::applicationDirPath())
		.arg(file_name);
}

void ThemeManager::installFileFonts(const QString& font_name_prefix, QList<QString>& ui_fallback_fonts) {
	const auto font_path = qFormat("%1/fonts/").arg(QCoreApplication::applicationDirPath());

    QMap<int32_t, QString> font_weight_map;
    QList<QString> file_paths;

    QDirIterator itr(font_path, QDir::Files | QDir::NoDotAndDotDot);

    while (itr.hasNext()) {
        auto file_path = itr.next();
        if (file_path.contains(font_name_prefix)) {
            file_paths.push_back(itr.fileName());
        }
    }

    sortFontWeight(file_paths.begin(), file_paths.end());

    Q_FOREACH(auto file_path, file_paths) {
        installFileFont(file_path, ui_fallback_fonts);
    }
}

void ThemeManager::installFileFont(const QString& file_name, QList<QString> &ui_fallback_fonts) {
	const auto font_path = fontNamePath(file_name);
	const QFileInfo info(font_path);
    if (!info.exists()) {
        XAMP_LOG_ERROR("Not found font file name: {}", file_name.toStdString());
        return;
    }

    const auto loaded_font_id = QFontDatabase::addApplicationFont(font_path);
    const auto font_families = QFontDatabase::applicationFontFamilies(loaded_font_id);
    if (!ui_fallback_fonts.contains(font_families[0])) {
        ui_fallback_fonts.push_back(font_families[0]);
    }
}

void ThemeManager::setSegoeFluentFontIcons() {
    HashMap<int32_t, uint32_t> glyphs_lut;

    QJsonDocument doc;
    if (json_util::deserializeFile("fonticon.json"_str, doc)) {
        auto jsonObject = doc.object();
        for (auto it = jsonObject.begin(); it != jsonObject.end(); ++it) {
            auto key = it.key().toStdString();
            auto value = it.value().toString();
            Glyphs glyph;
            if (FindGlyphs(key, glyph)) {
                glyphs_lut.insert(std::make_pair(static_cast<int32_t>(glyph), value.toUInt(nullptr, 16)));
            }
        }
    }

    qFontIcon.addFont(fontNamePath("FluentSystemIcons-Regular.ttf"_str));
    qFontIcon.setGlyphs(glyphs_lut);
}

void ThemeManager::setGoogleMaterialFontIcons() {
    HashMap<int32_t, uint32_t> glyphs_lut;

    QJsonDocument doc;
    if (json_util::deserializeFile("fonticon.json"_str, doc)) {
        auto jsonObject = doc.object();
        for (auto it = jsonObject.begin(); it != jsonObject.end(); ++it) {
            auto key = it.key().toStdString();
            auto value = it.value().toString();
            Glyphs glyph;
            if (FindGlyphs(key, glyph)) {
				glyphs_lut.insert(std::make_pair(static_cast<int32_t>(glyph), value.toUInt(nullptr, 16)));
            }
        }
    }

    qFontIcon.addFont(fontNamePath("MaterialIcons-Regular.ttf"_str));
    qFontIcon.setGlyphs(glyphs_lut);
}

ThemeManager& ThemeManager::GetInstance() {
	static ThemeManager instance;
	return instance;
}

QFont ThemeManager::loadFonts() {
    QList<QString> format_font;
    QList<QString> mono_fonts;
    QList<QString> display_fonts;
    QList<QString> ui_fonts;
    QList<QString> en_fonts;
    QList<QString> debug_fonts;

    installFileFonts("NotoSans"_str, mono_fonts);
    installFileFonts("FiraCode-Regular"_str, debug_fonts);
    installFileFont("Karla-Regular.ttf"_str, format_font);
    //installFileFonts("MiSansTC"), ui_fonts);
    //installFileFonts("MiSans"), ui_fonts);
    installFileFonts("HarmonyOS_Sans_TC"_str, ui_fonts);
    installFileFonts("HarmonyOS_Sans_SC"_str, ui_fonts);
    installFileFonts("ZenKakuGothicNew"_str, ui_fonts);

    sortFontWeight(ui_fonts.begin(), ui_fonts.end());

    if (display_fonts.isEmpty()) {
        display_fonts = ui_fonts;
    }
    if (mono_fonts.isEmpty()) {
        mono_fonts = ui_fonts;
    }

    QFont::insertSubstitutions("DebugFont"_str, debug_fonts);
    QFont::insertSubstitutions("DisplayFont"_str, display_fonts);
    QFont::insertSubstitutions("FormatFont"_str, format_font);
    QFont::insertSubstitutions("MonoFont"_str, mono_fonts);
    QFont::insertSubstitutions("UIFont"_str, ui_fonts);

    auto ui_font = uiFont();
    ui_font.setStyleStrategy(QFont::PreferAntialias);
    ui_font.setWeight(QFont::Weight::Normal);
    ui_font.setKerning(false);

    return ui_font;
}

void ThemeManager::setPalette() {
    palette_ = QPalette();
    if (theme_color_ == ThemeColor::LIGHT_THEME) {
        palette_.setColor(QPalette::WindowText, QColor(250, 250, 250));
        background_color_ = QColor(250, 250, 250);
    } else {
        palette_.setColor(QPalette::WindowText, QColor(25, 35, 45));
        background_color_ = QColor(40, 41, 42);
    }
}

ThemeManager::ThemeManager() {
    cover_size_ = QSize(185, 185);
    cache_cover_size_ = QSize(350, 350);
    album_cover_size_ = QSize(206, 206);
    save_cover_art_size_ = QSize(350, 350);
    ui_font_ = loadFonts();
#ifdef Q_OS_WIN
    font_ratio_ = 1.0;
#else
    font_ratio_ = 1.4;
#endif
    ui_font_.setPointSize(defaultFontSize());
    setGoogleMaterialFontIcons();
    //setSegoeFluentFontIcons();
}

void ThemeManager::setThemeColor(ThemeColor theme_color) {
    theme_color_ = theme_color;
    
    setPalette();

    qAppSettings.setEnumValue(kAppSettingTheme, theme_color_);

    font_icon_opts_.clear();

    switch (theme_color) {
    case ThemeColor::DARK_THEME:
        font_icon_opts_.insert(FontIconOption::kColorAttr, QVariant(QColor(240, 241, 243)));
        font_icon_opts_.insert(FontIconOption::kSelectedColorAttr, QVariant(QColor(240, 241, 243)));
        unknown_cover_ = QPixmap(":/xamp/Resource/Black/unknown_album.png"_str);
        break;
    case ThemeColor::LIGHT_THEME:
        font_icon_opts_.insert(FontIconOption::kColorAttr, QVariant(QColor(97, 97, 101)));
        font_icon_opts_.insert(FontIconOption::kSelectedColorAttr, QVariant(QColor(97, 97, 101)));
        unknown_cover_ = QPixmap(":/xamp/Resource/White/unknown_album.png"_str);
        break;
    }
    default_size_unknown_cover_ = image_util::resizeImage(unknown_cover_, album_cover_size_, true);
    emit themeChangedFinished(theme_color);
}

QLatin1String ThemeManager::themeColorPath() const {
    return themeColorPath(theme_color_);
}

QLatin1String ThemeManager::themeColorPath(ThemeColor theme_color) const {
    if (theme_color == ThemeColor::DARK_THEME) {
		return "Black"_str;
	}
	return "White"_str;
}

QColor ThemeManager::indicatorColor() const {
    return textColor();
}

QColor ThemeManager::textColor() const {
    auto color = Qt::black;
    switch (themeColor()) {
    case ThemeColor::DARK_THEME:
        color = Qt::white;
        break;
    case ThemeColor::LIGHT_THEME:
        color = Qt::black;
        break;
    }
    return color;
}

QColor ThemeManager::backgroundColor() const noexcept {
    return background_color_;
}

QString ThemeManager::backgroundColorString() const {
    QString color;

    switch (themeColor()) {
    case ThemeColor::DARK_THEME:
        color = "#19232D"_str;
        break;
    case ThemeColor::LIGHT_THEME:
        color = "#FAFAFA"_str;
        break;
    }
    return color;
}

void ThemeManager::setMenuStyle(QWidget* menu) {
	menu->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    menu->setAttribute(Qt::WA_TranslucentBackground);
    menu->setAttribute(Qt::WA_StyledBackground);
    menu->setStyle(new IconSizeStyle(14));
    auto f = defaultFont();
    f.setPointSize(10);
    menu->setFont(f);
}

QIcon ThemeManager::fontRawIcon(const Glyphs code) {
    return qFontIcon.getIcon(static_cast<int32_t>(code), font_icon_opts_);
}

QIcon ThemeManager::fontRawIconOption(const Glyphs code, const QVariantMap& options) {
    return qFontIcon.getIcon(static_cast<int32_t>(code), options);
}

QIcon ThemeManager::fontIcon(const Glyphs code, std::optional<ThemeColor> theme_color) {
    auto color = theme_color ? *theme_color : themeColor();

    switch (code) {
    case Glyphs::ICON_HEART_PRESS:
    case Glyphs::ICON_HEART:
    {
        auto temp = font_icon_opts_;
        if (code == Glyphs::ICON_HEART) {
            temp.insert(FontIconOption::kColorAttr, QVariant(QColor(128, 128, 128)));
        }
        else {
            temp.insert(FontIconOption::kColorAttr, QVariant(QColor(255, 0, 0)));
        }        
        return qFontIcon.getIcon(static_cast<int32_t>(code), temp);
    }
    case Glyphs::ICON_MINIMIZE_WINDOW:
	    {
        auto temp = font_icon_opts_;
        temp.insert(FontIconOption::kColorAttr, QVariant(color != ThemeColor::DARK_THEME ? QColor(Qt::black) : QColor(Qt::gray)));
        temp.insert(FontIconOption::kScaleFactorAttr,1.2);
        return qFontIcon.getIcon(static_cast<int32_t>(code), temp);
	    }
    case Glyphs::ICON_MAXIMUM_WINDOW:
        return QIcon(qFormat(":/xamp/Resource/%1/maximize-active.ico").arg(themeColorPath(color)));
    case Glyphs::ICON_CLOSE_WINDOW:
        return QIcon(qFormat(":/xamp/Resource/%1/close-active.ico").arg(themeColorPath(color)));
    case Glyphs::ICON_RESTORE_WINDOW:
        return QIcon(qFormat(":/xamp/Resource/%1/restore-active.ico").arg(themeColorPath(color)));
    case Glyphs::ICON_MESSAGE_BOX_WARNING:
	    {
			auto temp = font_icon_opts_;
            temp.insert(FontIconOption::kColorAttr, QVariant(QColor(255, 164, 6)));
            temp.insert(FontIconOption::kScaleFactorAttr, 0.8);
            return qFontIcon.getIcon(static_cast<int32_t>(code), temp);
	    }
    case Glyphs::ICON_MESSAGE_BOX_ERROR:
		{
			auto temp = font_icon_opts_;
			temp.insert(FontIconOption::kColorAttr, QVariant(QColor(189, 29, 29)));
			return qFontIcon.getIcon(static_cast<int32_t>(code), temp);
		}
    case Glyphs::ICON_MESSAGE_BOX_INFORMATION:
		{
			auto temp = font_icon_opts_;
			temp.insert(FontIconOption::kColorAttr, QVariant(QColor(43, 128, 234)));
		    return qFontIcon.getIcon(static_cast<int32_t>(code), temp);
		}
    case Glyphs::ICON_MESSAGE_BOX_QUESTION:
		{
			auto temp = font_icon_opts_;
			temp.insert(FontIconOption::kColorAttr, QVariant(QColor(53, 193, 31)));
			return qFontIcon.getIcon(static_cast<int32_t>(code), temp);
		}
    case Glyphs::ICON_MESSAGE_BOX_SUCCESS:
		{
			auto temp = font_icon_opts_;
			temp.insert(FontIconOption::kColorAttr, QVariant(QColor(0, 249, 0)));
			return qFontIcon.getIcon(static_cast<int32_t>(code), temp);
		}
    case Glyphs::ICON_CIRCLE_CHECK:
		{
			auto temp = font_icon_opts_;
			temp.insert(FontIconOption::kColorAttr, QVariant(highlightColor()));
			return qFontIcon.getIcon(static_cast<int32_t>(code), temp);
		}
    }
    return qFontIcon.getIcon(static_cast<int32_t>(code), font_icon_opts_);
}

QIcon ThemeManager::applicationIcon() const {
    return QIcon(":/xamp/xamp.ico"_str);
}

QIcon ThemeManager::playCircleIcon() const {
    return QIcon(":/xamp/Resource/Black/play_circle.png"_str);
}

QIcon ThemeManager::playlistPauseIcon(QSize icon_size, double scale_factor) const {
    QVariantMap font_options;
    font_options.insert(FontIconOption::kScaleFactorAttr, QVariant::fromValue(scale_factor));
    font_options.insert(FontIconOption::kColorAttr, QColor(250, 88, 106));
    font_options.insert(FontIconOption::kSelectedColorAttr, QColor(250, 88, 106));

    auto icon = qFontIcon.getIcon(static_cast<int32_t>(Glyphs::ICON_PLAY_LIST_PAUSE), font_options);
    icon.addPixmap(icon.pixmap(icon_size, QIcon::Normal, QIcon::Off),
        QIcon::Selected, QIcon::Off);
    icon.addPixmap(icon.pixmap(icon_size, QIcon::Normal, QIcon::On),
        QIcon::Selected, QIcon::On);
    return icon;
}

QIcon ThemeManager::playlistPlayingIcon(QSize icon_size, double scale_factor) const {
    QVariantMap font_options;
    font_options.insert(FontIconOption::kScaleFactorAttr, QVariant::fromValue(scale_factor));
    font_options.insert(FontIconOption::kColorAttr, QColor(250, 88, 106));
    font_options.insert(FontIconOption::kSelectedColorAttr, QColor(250, 88, 106));
    auto icon = qFontIcon.getIcon(static_cast<int32_t>(Glyphs::ICON_PLAY_LIST_PLAY), font_options);

    icon.addPixmap(icon.pixmap(icon_size, QIcon::Normal, QIcon::Off),
        QIcon::Selected, QIcon::Off);
    icon.addPixmap(icon.pixmap(icon_size, QIcon::Normal, QIcon::On),
        QIcon::Selected, QIcon::On);
    return icon;
}

QIcon ThemeManager::playingIcon() const {
    QVariantMap font_options;
    font_options.insert(FontIconOption::kColorAttr, QColor(252, 215, 75));
    return qFontIcon.getIcon(static_cast<int32_t>(Glyphs::ICON_PLAYING), font_options);
}

QIcon ThemeManager::hdIcon() const {
    QVariantMap font_options;
    font_options.insert(FontIconOption::kColorAttr, QColor(252, 215, 75));
    return qFontIcon.getIcon(static_cast<int32_t>(Glyphs::ICON_HD_AUDIO), font_options);
}

QPixmap ThemeManager::githubIcon() const {
    if (themeColor() == ThemeColor::DARK_THEME) {
        return QPixmap(":/xamp/Resource/Black/GitHub-Mark.png"_str);
	} else {
        return QPixmap(":/xamp/Resource/White/GitHub-Mark.png"_str);
	}
}

const QPixmap& ThemeManager::unknownCover() noexcept {
    return unknown_cover_;
}

const QPixmap& ThemeManager::defaultSizeUnknownCover() noexcept {
    return default_size_unknown_cover_;
}

void ThemeManager::updateMaximumIcon(QToolButton *maxWinButton, bool is_maximum) {
    if (is_maximum) {
        maxWinButton->setIcon(fontIcon(Glyphs::ICON_RESTORE_WINDOW));        
    } else {
        maxWinButton->setIcon(fontIcon(Glyphs::ICON_MAXIMUM_WINDOW));
    }
}

void ThemeManager::setHeartButton(QToolButton* heartButton, bool press) {
    heartButton->setIcon(fontIcon(press ? Glyphs::ICON_HEART_PRESS : Glyphs::ICON_HEART));
    heartButton->setStyleSheet("background: transparent;"_str);
}

void ThemeManager::setPlayOrPauseButton(QToolButton *playButton, bool is_playing) {
    if (is_playing) {
        playButton->setIcon(fontIcon(Glyphs::ICON_PAUSE));
    }
    else {
        playButton->setIcon(fontIcon(Glyphs::ICON_PLAY));
    }
}

const QSize& ThemeManager::defaultCoverSize() const noexcept {
    return cover_size_;
}

QSize ThemeManager::cacheCoverSize() const noexcept {
    return cache_cover_size_;
}

QSize ThemeManager::albumCoverSize() const noexcept {
    return album_cover_size_;
}

void ThemeManager::setBackgroundColor(QWidget* widget) {
    auto color = palette().color(QPalette::WindowText);
    widget->setStyleSheet(backgroundColorToString(color));
}

void ThemeManager::setThemeQssFile() {
    qApp->setFont(defaultFont());

    QString filename;

    if (themeColor() == ThemeColor::LIGHT_THEME) {
        filename = ":/xamp/Resource/Theme/light/style.qss"_str;
    } else {
        filename = ":/xamp/Resource/Theme/dark/style.qss"_str;
    }

    QFile f(filename);
    f.open(QFile::ReadOnly | QFile::Text);
    QTextStream ts(&f);
    ts.setEncoding(QStringConverter::Utf8);
    qApp->setStyleSheet(ts.readAll());

    f.close();
}

void ThemeManager::setBackgroundColor(QColor color) {
    background_color_ = color;
    qAppSettings.setValue(kAppSettingBackgroundColor, color);
}

QColor ThemeManager::titleBarColor() const {
    return QColor(backgroundColor());
}

QColor ThemeManager::coverShadowColor() const {
    switch (themeColor()) {
    case ThemeColor::DARK_THEME:
    case ThemeColor::LIGHT_THEME:
    default:
        return {"#DCDCDC"_str};
    }
}

QString ThemeManager::linearGradientStyle() const {
    switch (themeColor()) {
    default:
    case ThemeColor::DARK_THEME:
        return "#2e2f31"_str;
    case ThemeColor::LIGHT_THEME:
        return "#ffffff"_str;
    }
}

QSize ThemeManager::tabIconSize() const {
#ifdef XAMP_OS_MAC
    return QSize(20, 20);
#else
    return QSize(18, 18);
#endif
}

QColor ThemeManager::hoverColor() const {
    switch (themeColor()) {
    case ThemeColor::DARK_THEME:
        return {"#43474e"_str };
    case ThemeColor::LIGHT_THEME:
    default:
        return {"#C9CDD0"_str };
    }
}

QColor ThemeManager::highlightColor() const {
    switch (themeColor()) {
    case ThemeColor::LIGHT_THEME:
        return {"#9FCBFF"_str};
    case ThemeColor::DARK_THEME:
    default:
        return {"#1A72BB"_str };
    }
}

int32_t ThemeManager::titleBarIconHeight() {
    return 8;
}

void ThemeManager::setTitleBarButtonStyle(QToolButton* close_button, QToolButton* min_win_button, QToolButton* max_win_button) {
    const QColor hover_color = hoverColor();
    const QColor color_hover_color("#dc3545"_str);

    close_button->setStyleSheet(qFormat(R"(
                                         QToolButton#closeButton {
                                         border: none;
                                         background-color: transparent;
										 border-radius: 0px;
                                         }

										 QToolButton#closeButton:hover {
										 background-color: %1;
										 border-radius: 0px;
                                         }
                                         )").arg(colorToString(color_hover_color)));
    close_button->setIconSize(QSize(titleBarIconHeight(), titleBarIconHeight()));
    close_button->setIcon(fontIcon(Glyphs::ICON_CLOSE_WINDOW));

    min_win_button->setStyleSheet(qFormat(R"(
                                          QToolButton#minWinButton {
                                          border: none;
                                          background-color: transparent;
										  border-radius: 0px;
                                          }
										  QToolButton#minWinButton:hover {		
										  background-color: %1;
										  border-radius: 0px;				 
                                          }
                                          )").arg(colorToString(hover_color)));
    min_win_button->setIconSize(QSize(titleBarIconHeight(), titleBarIconHeight()));
    min_win_button->setIcon(fontIcon(Glyphs::ICON_MINIMIZE_WINDOW));
    
    max_win_button->setStyleSheet(qFormat(R"(
                                          QToolButton#maxWinButton {
                                          border: none;
                                          background-color: transparent;
										  border-radius: 0px;
                                          }
										  QToolButton#maxWinButton:hover {		
										  background-color: %1;
										  border-radius: 0px;								 
                                          }
                                          )").arg(colorToString(hover_color)));
    max_win_button->setIconSize(QSize(titleBarIconHeight(), titleBarIconHeight()));
    max_win_button->setIcon(fontIcon(Glyphs::ICON_MAXIMUM_WINDOW));
}

void ThemeManager::setFrameBackgroundColor(QFrame* frame) {
    switch (themeColor()) {
    case ThemeColor::DARK_THEME:
        frame->setStyleSheet(qFormat("QFrame#%1 { background-color: #1c1c1e; }").arg(frame->objectName()));
        break;
    case ThemeColor::LIGHT_THEME:
        frame->setStyleSheet(qFormat("QFrame#%1 { background-color: #CED1D4; }").arg(frame->objectName()));
        break;
    }
}

int32_t ThemeManager::fontSize(int32_t base_size) const {
    return base_size * font_ratio_;
}

int32_t ThemeManager::defaultFontSize() const {
#ifdef Q_OS_WIN
    return fontSize(10);
#else
    return fontSize(12);
#endif
}

QSize ThemeManager::titleButtonIconSize() {
    return {titleBarIconHeight(), titleBarIconHeight()};
}

QFont ThemeManager::formatFont() const {
    return QFont("FormatFont"_str);
}

QFont ThemeManager::uiFont() const {
    return QFont("UIFont"_str);
}

QFont ThemeManager::displayFont() const {
    return QFont("DisplayFont"_str);
}

QFont ThemeManager::monoFont() const {
    return QFont("MonoFont"_str);
}

QFont ThemeManager::debugFont() const {
    return QFont("DebugFont"_str);
}

void ThemeManager::setRecordIcon(QToolButton* record_button, bool is_recording) {
    if (!is_recording) {
        record_button->setIcon(fontIcon(Glyphs::ICON_MIC));
    }
    else {
        record_button->setIcon(fontIcon(Glyphs::ICON_SEND));
    }
}

void ThemeManager::setCancelRecordIcon(QToolButton* cancel_button) {
    cancel_button->setIcon(fontIcon(Glyphs::ICON_CLOSE_WINDOW));
}

void ThemeManager::setComboBoxStyle(QComboBox* combo_box, const QString& object_name) {
    QString border_color;
    QString selection_background_color;
    QString on_selection_background_color;

    switch (themeColor()) {
    case ThemeColor::LIGHT_THEME:
        border_color = "#C9CDD0"_str;
        selection_background_color = "#FAFAFA"_str;
        on_selection_background_color = "#1e1d23"_str;
        break;
    case ThemeColor::DARK_THEME:
        border_color = "#455364"_str;
        selection_background_color = "#1e1d23"_str;
        on_selection_background_color = "#9FCBFF"_str;
        break;
    }

    combo_box->setStyleSheet(qFormat(R"(
    QComboBox#%4 {
		background-color: %2;
		border: 1px solid %1;
	}
	QComboBox QAbstractItemView#%4 {
		background-color: %2;
	}
	QComboBox#%4:on {
		selection-background-color: %3;
	}
    )").arg(border_color)
       .arg(selection_background_color)
       .arg(on_selection_background_color)
       .arg(object_name)
    );
}

void ThemeManager::setLineEditStyle(QLineEdit* line_edit, const QString& object_name) {
    switch (themeColor()) {
        case ThemeColor::DARK_THEME:
            line_edit->setStyleSheet(qFormat(R"(
                                            QLineEdit#%1 {
                                            background-color: %2;
                                            border: 1px solid #4d4d4d;
                                            color: white;
                                            border-radius: 8px;
                                            }
                                            )").arg(object_name).arg("#3a3a3c"_str));
			break;
        case ThemeColor::LIGHT_THEME:
            line_edit->setStyleSheet(qFormat(R"(
                                            QLineEdit#%1 {
                                            background-color: %2;
                                            border: 1px solid gray;
                                            color: black;
                                            border-radius: 8px;
                                            }
                                            )").arg(object_name).arg(colorToString(Qt::white)));
            break;
    }
}

void ThemeManager::setMuted(QAbstractButton *button, bool is_muted) {
    if (!is_muted) {
        button->setIcon(fontIcon(Glyphs::ICON_VOLUME_UP));
        qAppSettings.setValue<bool>(kAppSettingIsMuted, false);
    }
    else {
        button->setIcon(fontIcon(Glyphs::ICON_VOLUME_OFF));
        qAppSettings.setValue(kAppSettingIsMuted, true);
    }
}

void ThemeManager::setVolume(QSlider *slider, QAbstractButton* button, uint32_t volume) {
    if (!slider->isEnabled()) {
        return;
    }
    if (volume == 0) {
        setMuted(button, true);
    }
    else {
        setMuted(button, false);
    }
    slider->setValue(volume);
}

Glyphs ThemeManager::connectTypeGlyphs(DeviceConnectType type) const {
    switch (type) {
    case DeviceConnectType::UNKNOWN:
    case DeviceConnectType::BUILT_IN_SPEAKER:
        return Glyphs::ICON_BUILD_IN_SPEAKER;
    case DeviceConnectType::USB:
        return Glyphs::ICON_USB;
    case DeviceConnectType::BLUE_TOOTH:
        return Glyphs::ICON_BLUE_TOOTH;
    }
    return Glyphs::ICON_BUILD_IN_SPEAKER;
}

QIcon ThemeManager::connectTypeIcon(DeviceConnectType type) {
    switch (type) {
    case DeviceConnectType::UNKNOWN:
    case DeviceConnectType::BUILT_IN_SPEAKER:
        return fontIcon(Glyphs::ICON_BUILD_IN_SPEAKER);
    case DeviceConnectType::USB:
        return fontIcon(Glyphs::ICON_USB);
    case DeviceConnectType::BLUE_TOOTH:
        return fontIcon(Glyphs::ICON_BLUE_TOOTH);
    }
    return fontIcon(Glyphs::ICON_BUILD_IN_SPEAKER);
}

void ThemeManager::setDeviceConnectTypeIcon(QAbstractButton* button, DeviceConnectType type) {
    button->setIcon(connectTypeIcon(type));
    button->update();
}

void ThemeManager::setSliderTheme(QSlider* slider, bool enter) {
    QString slider_background_color;
    QString slider_border_color;

    switch (themeColor()) {
    case ThemeColor::LIGHT_THEME:
        slider_background_color = "#9FCBFF"_str;
        slider_border_color = "#C9CDD0"_str;
        break;
    case ThemeColor::DARK_THEME:
        slider_background_color = "#1A72BB"_str;
        slider_border_color = "#43474e"_str;
        break;
    }

    // handle 預設的邊框顏色
    auto handle_border_color = slider_background_color;

    // 原先的 margin 值
    auto margin = 10;
    if (!enter) {
        handle_border_color = "transparent"_str;
        margin = 1;
    }

    // 讓 handle 更圓：假設 handle 為 12x12，border-radius 設為 6 (直徑的一半)
    // 可以依需求自行調整大小
    const int handleSize = 12;
    const int handleRadius = handleSize / 2;

    slider->setStyleSheet(qFormat(R"(
    QSlider {
        background-color: transparent;
    }

    QSlider#%1::groove:vertical {        
        background: %2;
        border: 1px solid %2;
        width: 2px;
        border-radius: 2px;
        padding-top: -1px;
        padding-bottom: 0px;
    }

    QSlider#%1::sub-page:vertical {
        background: %3;
        border: 1px solid %3;
        width: 2px;
        border-radius: 2px;
    }

    QSlider#%1::add-page:vertical {
        background: %2;
        border: 0px solid %2;
        width: 2px;
        border-radius: 2px;
    }

    QSlider#%1::handle:vertical {
        /* 設為相同的寬高 + 圓角，視覺上就會是圓 */
        width: %5px;
        height: %5px;
        margin: 0px -%6px 0px -%6px;  /* 可視需求微調 */
        background-color: %2;
        border: 1px solid %2;
        border-radius: %7px;
    }

    QSlider#%1::groove:horizontal {        
        background: %2;
        border: 1px solid %2;
        height: 2px;
        border-radius: 2px;
        padding-left: -1px;
        padding-right: 1px;
    }

    QSlider#%1::sub-page:horizontal {
        background: %2;
        border: 1px solid %2;
        height: 2px;
        border-radius: 2px;
    }

    QSlider#%1::add-page:horizontal {
        background: %3;
        border: 0px solid %2;
        height: 2px;
        border-radius: 2px;
    }

    QSlider#%1::handle:horizontal {
        width: %5px;
        height: %5px;
        margin: -%6px 0px -%6px 0px; /* 可視需求微調 */
        border-radius: %7px;
        background-color: %4;
        border: 1px solid %4;
    }
    )"
    ).arg(slider->objectName())
        .arg(slider_background_color)
        .arg(slider_border_color)
        .arg(handle_border_color)
        .arg(handleSize)
        // margin 以 handleSize / 2 做微調，讓整個 handle 在中線上
        .arg(handleSize / 2)
        .arg(handleRadius)
    );
}

void ThemeManager::setAlbumNaviBarTheme(QListView *tab) const {
    QString tab_left_color;

    switch (themeColor()) {
    case ThemeColor::DARK_THEME:
        tab_left_color = "42, 130, 218"_str;
        break;
    case ThemeColor::LIGHT_THEME:
        tab_left_color = "42, 130, 218"_str;
        break;
    }

    tab->setStyleSheet(qFormat(R"(
	QListView#albumTab {
		border: none; 
	}
	QListView#albumTab::item {
		border: 0px;
		padding: 2px;
	}
	QListView#albumTab::item:hover {
		background-color: transparent;
		border-radius: 2px;
	}
	QListView#albumTab::item:selected {
		padding: 2px; 
		background-color: transparent;
        color: rgb(%2);
		border-bottom-width: 2px;
		border-bottom-style: solid;
		border-bottom-color: rgb(%1);
	}	
	)").arg(tab_left_color).arg(tab_left_color));
}

