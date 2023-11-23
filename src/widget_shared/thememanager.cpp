#include <thememanager.h>

#include <widget/image_utiltis.h>
#if defined(Q_OS_WIN)
#include <widget/win32/wintaskbar.h>
#else
#include <widget/osx/osx.h>
#endif

#include <widget/appsettingnames.h>
#include <widget/widget_shared.h>
#include <widget/str_utilts.h>
#include <widget/appsettings.h>
#include <widget/iconsizestyle.h>

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
    void SortFontWeight(Iterator begin, Iterator end) {
        std::sort(begin, end,
            [](const auto& left_font_name, const auto& right_font_name) {
                auto getFontWeight = [](auto name) {
                    if (name.contains(qTEXT("Thin"), Qt::CaseInsensitive)) {
                        return 100 * 10;
                    }
                    if (name.contains(qTEXT("Hairline"), Qt::CaseInsensitive)) {
                        return 100 * 10;
                    }
                    if (name.contains(qTEXT("ExtraLight"), Qt::CaseInsensitive)) {
                        return 200 * 10;
                    }
                    if (name.contains(qTEXT("Light"), Qt::CaseInsensitive)) {
                        return 300 * 10;
                    }
                    if (name.contains(qTEXT("Normal"), Qt::CaseInsensitive)) {
                        return 400;
                    }
                    if (name.contains(qTEXT("Regular"), Qt::CaseInsensitive)) {
                        return 400;
                    }
                    if (name.contains(qTEXT("Medium"), Qt::CaseInsensitive)) {
                        return 500;
                    }
                    if (name.contains(qTEXT("SemiBold"), Qt::CaseInsensitive)) {
                        return 600;
                    }
                    if (name.contains(qTEXT("DemiBold"), Qt::CaseInsensitive)) {
                        return 600;
                    }
                    if (name.contains(qTEXT("Bold"), Qt::CaseInsensitive)) {
                        return 700;
                    }
                    if (name.contains(qTEXT("ExtraBold"), Qt::CaseInsensitive)) {
                        return 800;
                    }
                    if (name.contains(qTEXT("UltraBold"), Qt::CaseInsensitive)) {
                        return 800;
                    }
                    if (name.contains(qTEXT("Black"), Qt::CaseInsensitive)) {
                        return 900;
                    }
                    if (name.contains(qTEXT("Heavy"), Qt::CaseInsensitive)) {
                        return 900;
                    }
                    return 100;
                    };
                return getFontWeight(left_font_name) < getFontWeight(right_font_name);
            });
    }

    QIcon UniformIcon(QIcon icon, QSize size) {
        QIcon result;
        const auto base_pixmap = icon.pixmap(size);
        for (const auto state : { QIcon::Off, QIcon::On }) {
            for (const auto mode : { QIcon::Normal, QIcon::Disabled, QIcon::Active, QIcon::Selected })
                result.addPixmap(base_pixmap, mode, state);
        }
        return result;
    }
}

QString ThemeManager::GetCountryFlagFilePath(const QString& country_iso_code) {
    return
        qSTR("%1/flags/%2.png")
        .arg(QCoreApplication::applicationDirPath())
        .arg(country_iso_code);
}

QString ThemeManager::GetFontNamePath(const QString& file_name) {
	return
		qSTR("%1/fonts/%2")
		.arg(QCoreApplication::applicationDirPath())
		.arg(file_name);
}

void ThemeManager::InstallFileFonts(const QString& font_name_prefix, QList<QString>& ui_fallback_fonts) {
	const auto font_path = qSTR("%1/fonts/").arg(QCoreApplication::applicationDirPath());

    QMap<int32_t, QString> font_weight_map;
    QList<QString> file_paths;

    QDirIterator itr(font_path, QDir::Files | QDir::NoDotAndDotDot);

    while (itr.hasNext()) {
        auto file_path = itr.next();
        if (file_path.contains(font_name_prefix)) {
            file_paths.push_back(itr.fileName());
        }
    }

    SortFontWeight(file_paths.begin(), file_paths.end());

    Q_FOREACH(auto file_path, file_paths) {
        InstallFileFont(file_path, ui_fallback_fonts);
    }
}

void ThemeManager::InstallFileFont(const QString& file_name, QList<QString> &ui_fallback_fonts) {
	const auto font_path = GetFontNamePath(file_name);
	const QFileInfo info(font_path);
    if (!info.exists()) {
        XAMP_LOG_ERROR("Not found font file name: {}", file_name.toStdString());
        return;
    }
    PrefetchFile(font_path.toStdWString());
    const auto loaded_font_id = QFontDatabase::addApplicationFont(font_path);
    const auto font_families = QFontDatabase::applicationFontFamilies(loaded_font_id);
    if (!ui_fallback_fonts.contains(font_families[0])) {
        ui_fallback_fonts.push_back(font_families[0]);
    }
}

void ThemeManager::SetFontAwesomeIcons() {
    const HashMap<char32_t, uint32_t> glyphs{
    { ICON_VOLUME_UP ,                0xF6A8 },
    { ICON_VOLUME_OFF,                0xF6A9 },
    { ICON_SPEAKER,                   0xF8DF },
    { ICON_FOLDER,                    0xF07B },
    { ICON_AUDIO,                     0xF001 },
    { ICON_LOAD_FILE,                 0xF15B },
    { ICON_LOAD_DIR,                  0xF07C },
    { ICON_RELOAD,                    0xF2F9 },
    { ICON_REMOVE_ALL,                0xE2AE },
    { ICON_OPEN_FILE_PATH,            0xF07C },
    { ICON_SCAN_REPLAY_GAIN,          0xF5F0 },
    { ICON_EXPORT_FILE,               0xF56E },
    { ICON_COPY,                      0xF0C5 },
    { ICON_DOWNLOAD,                  0xF0ED },
    { ICON_PLAYLIST,                  0xF8C9 },
    { ICON_PLAYLIST_ORDER,            0xF0CA },
    { ICON_EQUALIZER,                 0xF3F2 },
    { ICON_PODCAST,                   0xF2CE },
    { ICON_MUSIC_LIBRARY,             0xF001 },
    { ICON_CD,                        0xF51F },
    { ICON_LEFT_ARROW,                0xF177 },
    { ICON_ARTIST,                    0xF500 },
    { ICON_SUBTITLE,                  0xE1DE },
    { ICON_SETTINGS,                  0xF013 },
    { ICON_ABOUT,                     0xF05A },
    { ICON_DARK_MODE,                 0xF186 },
    { ICON_LIGHT_MODE,                0xF185 },
    { ICON_SEARCH,                    0xF002 },
    { ICON_THEME,                     0xF042 },
    { ICON_DESKTOP,                   0xF390 },
    { ICON_SHUFFLE_PLAY_ORDER,        0xF074 },
    { ICON_REPEAT_ONE_PLAY_ORDER,     0xF363 },
    { ICON_REPEAT_ONCE_PLAY_ORDER,    0xF365 },
    { ICON_MINIMIZE_WINDOW,           0xF2D1 },
    { ICON_MAXIMUM_WINDOW,            0xF2D0 },
    { ICON_CLOSE_WINDOW,              0xF00D },
    { ICON_RESTORE_WINDOW,            0xF2D2 },
    { ICON_SLIDER_BAR,                0xF0C9 },
    { ICON_PLAY_LIST_PLAY,            0xF04B },
    { ICON_PLAY_LIST_PAUSE,           0xF04C },
    { ICON_PLAY,                      0xF144 },
    { ICON_PAUSE,                     0xF28B },
    { ICON_STOP_PLAY,                 0xF04D },
    { ICON_PLAY_FORWARD,              0xF04E },
    { ICON_PLAY_BACKWARD,             0xF04A },
    { ICON_MORE,                      0xF142 },
    { ICON_HIDE,                      0xF070 },
    { ICON_SHOW,                      0xF06E },
    { ICON_USB,                       0xF8E9 },
    { ICON_BUILD_IN_SPEAKER,          0xF8DF },
    { ICON_BLUE_TOOTH,                0xF293 },
    { ICON_MESSAGE_BOX_WARNING,       0xF071 },
    { ICON_MESSAGE_BOX_INFORMATION,   0xF05A },
    { ICON_MESSAGE_BOX_ERROR,         0xF05E },
    { ICON_MESSAGE_BOX_QUESTION,      0xF059 },
    { ICON_MESSAGE_BOX_SUCCESS,       0xF058 },
    { ICON_HEART,                     0xF004 },
    { ICON_HEART_PRESS,               0xF004 },
    { ICON_CHEVRON_RIGHT,             0xF054 },
    { ICON_CHEVRON_LEFT,              0xF053 },
    { ICON_SORT_DOWN,                 0xF160 },
    { ICON_FILE_CIRCLE_PLUS,          0xE494 },
    { ICON_UP,                        0xF357 },
    { ICON_EDIT,                      0xF044 },
    { ICON_CIRCLE_CHECK,              0xF058 }
    };
    
    switch (theme_color_) {
    case ThemeColor::LIGHT_THEME:
        qFontIcon.AddFont(GetFontNamePath(qTEXT("fa-regular-400.ttf")));
        break;
    case ThemeColor::DARK_THEME:
        qFontIcon.AddFont(GetFontNamePath(qTEXT("fa-solid-900.ttf")));
        break;
    }   

    qFontIcon.SetGlyphs(glyphs);
}

QFont ThemeManager::LoadFonts() {
    QList<QString> format_font;
    QList<QString> mono_fonts;
    QList<QString> display_fonts;
    QList<QString> ui_fonts;
    QList<QString> en_fonts;
    QList<QString> debug_fonts;    

    InstallFileFont(qTEXT("Karla-Regular.ttf"), format_font);
    InstallFileFonts(qTEXT("NotoSans"), mono_fonts);
    InstallFileFonts(qTEXT("OpenSans"), en_fonts);
    InstallFileFonts(qTEXT("MiSans"), ui_fonts);
    InstallFileFonts(qTEXT("FiraCode-Regular"), debug_fonts);

    SortFontWeight(ui_fonts.begin(), ui_fonts.end());
    ui_fonts.insert(0, en_fonts[0]);
    ui_fonts.insert(1, en_fonts[1]);
    
    if (display_fonts.isEmpty()) {
        display_fonts = ui_fonts;
    }
    if (mono_fonts.isEmpty()) {
        mono_fonts = ui_fonts;
    }

    QFont::insertSubstitutions(qTEXT("DebugFont"), debug_fonts);
    QFont::insertSubstitutions(qTEXT("DisplayFont"), display_fonts);
    QFont::insertSubstitutions(qTEXT("FormatFont"), format_font);
    QFont::insertSubstitutions(qTEXT("MonoFont"), mono_fonts);
    QFont::insertSubstitutions(qTEXT("UIFont"), ui_fonts);

    auto ui_font = GetUiFont();
    ui_font.setStyleStrategy(QFont::PreferAntialias);
#ifdef Q_OS_WIN
    ui_font.setWeight(QFont::Weight::Medium);
#else
    ui_font.setWeight(QFont::Weight::Normal);
#endif
    ui_font.setKerning(false);

    return ui_font;
}

void ThemeManager::SetPalette() {
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
    ui_font_ = LoadFonts();
    const auto* screen = qApp->screens()[0];
    const auto screen_size = screen->size();
    if (screen_size.width() >= 1920 && screen_size.height() == 1080) {
        font_ratio_ = 1.25;
    }
    else {
        font_ratio_ = 1.0;
    }
    ui_font_.setPointSize(GetDefaultFontSize());
}

void ThemeManager::SetThemeColor(ThemeColor theme_color) {
    theme_color_ = theme_color;

    SetFontAwesomeIcons();
    SetPalette();

    qAppSettings.SetEnumValue(kAppSettingTheme, theme_color_);

    font_icon_opts_.clear();

    switch (theme_color) {
    case ThemeColor::DARK_THEME:
        font_icon_opts_.insert(FontIconOption::colorAttr, QVariant(QColor(240, 241, 243)));
        font_icon_opts_.insert(FontIconOption::selectedColorAttr, QVariant(QColor(240, 241, 243)));
        unknown_cover_ = QPixmap(qTEXT(":/xamp/Resource/Black/unknown_album.png"));
        break;
    case ThemeColor::LIGHT_THEME:
        font_icon_opts_.insert(FontIconOption::colorAttr, QVariant(QColor(97, 97, 101)));
        font_icon_opts_.insert(FontIconOption::selectedColorAttr, QVariant(QColor(97, 97, 101)));
        unknown_cover_ = QPixmap(qTEXT(":/xamp/Resource/White/unknown_album.png"));
        break;
    }
    default_size_unknown_cover_ = image_utils::ResizeImage(unknown_cover_, album_cover_size_, true);
}

QLatin1String ThemeManager::GetThemeColorPath() const {
    return GetThemeColorPath(theme_color_);
}

QLatin1String ThemeManager::GetThemeColorPath(ThemeColor theme_color) const {
    if (theme_color == ThemeColor::DARK_THEME) {
		return qTEXT("Black");
	}
	return qTEXT("White");
}

QColor ThemeManager::GetThemeTextColor() const {
    auto color = Qt::black;
    switch (GetThemeColor()) {
    case ThemeColor::DARK_THEME:
        color = Qt::white;
        break;
    case ThemeColor::LIGHT_THEME:
        color = Qt::black;
        break;
    }
    return color;
}

QColor ThemeManager::GetBackgroundColor() const noexcept {
    return background_color_;
}

QString ThemeManager::GetBackgroundColorString() const {
    QString color;

    switch (GetThemeColor()) {
    case ThemeColor::DARK_THEME:
        color = qTEXT("#19232D");
        break;
    case ThemeColor::LIGHT_THEME:
        color = qTEXT("#FAFAFA");
        break;
    }
    return color;
}

void ThemeManager::SetMenuStyle(QWidget* menu) {
	menu->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    menu->setAttribute(Qt::WA_TranslucentBackground);
    menu->setAttribute(Qt::WA_StyledBackground);
    menu->setStyle(new IconSizeStyle(12));
}

QIcon ThemeManager::GetFontIcon(const char32_t& code, QVariantMap options) {
    return qFontIcon.GetIcon(code, options);
}

QIcon ThemeManager::GetFontIcon(const char32_t code, std::optional<ThemeColor> theme_color) const {
    auto color = theme_color ? *theme_color : GetThemeColor();

    switch (code) {
    case Glyphs::ICON_HEART_PRESS:
    case Glyphs::ICON_HEART:
    {
        auto temp = font_icon_opts_;
        if (code == Glyphs::ICON_HEART) {
            temp.insert(FontIconOption::colorAttr, QVariant(QColor(128, 128, 128)));
        }
        else {
            temp.insert(FontIconOption::colorAttr, QVariant(QColor(255, 0, 0)));
        }        
        return qFontIcon.GetIcon(code, temp);
    }
    case Glyphs::ICON_MINIMIZE_WINDOW:
	    {
        auto temp = font_icon_opts_;
        temp.insert(FontIconOption::colorAttr, QVariant(color != ThemeColor::DARK_THEME ? QColor(Qt::black) : QColor(Qt::gray)));
        return qFontIcon.GetIcon(0xF2D1, temp);
	    }
    case Glyphs::ICON_MAXIMUM_WINDOW:
        return QIcon(qSTR(":/xamp/Resource/%1/maximize-active.ico").arg(GetThemeColorPath(color)));
    case Glyphs::ICON_CLOSE_WINDOW:
        return QIcon(qSTR(":/xamp/Resource/%1/close-active.ico").arg(GetThemeColorPath(color)));
    case Glyphs::ICON_RESTORE_WINDOW:
        return QIcon(qSTR(":/xamp/Resource/%1/restore-active.ico").arg(GetThemeColorPath(color)));
    case Glyphs::ICON_MESSAGE_BOX_WARNING:
	    {
			auto temp = font_icon_opts_;
            temp.insert(FontIconOption::colorAttr, QVariant(QColor(255, 164, 6)));
            return qFontIcon.GetIcon(code, temp);
	    }
    case Glyphs::ICON_MESSAGE_BOX_ERROR:
		{
			auto temp = font_icon_opts_;
			temp.insert(FontIconOption::colorAttr, QVariant(QColor(189, 29, 29)));
			return qFontIcon.GetIcon(code, temp);
		}
    case Glyphs::ICON_MESSAGE_BOX_INFORMATION:
		{
			auto temp = font_icon_opts_;
			temp.insert(FontIconOption::colorAttr, QVariant(QColor(43, 128, 234)));
		    return qFontIcon.GetIcon(code, temp);
		}
    case Glyphs::ICON_MESSAGE_BOX_QUESTION:
		{
			auto temp = font_icon_opts_;
			temp.insert(FontIconOption::colorAttr, QVariant(QColor(53, 193, 31)));
			return qFontIcon.GetIcon(code, temp);
		}
    case Glyphs::ICON_MESSAGE_BOX_SUCCESS:
		{
			auto temp = font_icon_opts_;
			temp.insert(FontIconOption::colorAttr, QVariant(QColor(0, 249, 0)));
			return qFontIcon.GetIcon(code, temp);
		}
    case Glyphs::ICON_CIRCLE_CHECK:
		{
			auto temp = font_icon_opts_;
			temp.insert(FontIconOption::colorAttr, QVariant(GetHighlightColor()));
			return qFontIcon.GetIcon(code, temp);
		}
    }
    return qFontIcon.GetIcon(code, font_icon_opts_);
}

QIcon ThemeManager::GetApplicationIcon() const {
    return QIcon(qTEXT(":/xamp/xamp.ico"));
}

QIcon ThemeManager::GetPlayCircleIcon() const {
    return QIcon(qTEXT(":/xamp/Resource/Black/play_circle.png"));
}

QIcon ThemeManager::PlaylistPauseIcon(QSize icon_size) const {
    QVariantMap font_options;
    font_options.insert(FontIconOption::scaleFactorAttr, QVariant::fromValue(0.3));
    font_options.insert(FontIconOption::colorAttr, QColor(250, 88, 106));
    font_options.insert(FontIconOption::selectedColorAttr, QColor(250, 88, 106));

    auto icon = qFontIcon.GetIcon(Glyphs::ICON_PLAY_LIST_PAUSE, font_options);
    icon.addPixmap(icon.pixmap(icon_size, QIcon::Normal, QIcon::Off),
        QIcon::Selected, QIcon::Off);
    icon.addPixmap(icon.pixmap(icon_size, QIcon::Normal, QIcon::On),
        QIcon::Selected, QIcon::On);
    return icon;
}

QIcon ThemeManager::GetPlaylistPlayingIcon(QSize icon_size) const {
    QVariantMap font_options;
    font_options.insert(FontIconOption::scaleFactorAttr, QVariant::fromValue(0.3));
    font_options.insert(FontIconOption::colorAttr, QColor(250, 88, 106));
    font_options.insert(FontIconOption::selectedColorAttr, QColor(250, 88, 106));
    auto icon = qFontIcon.GetIcon(Glyphs::ICON_PLAY_LIST_PLAY, font_options);

    icon.addPixmap(icon.pixmap(icon_size, QIcon::Normal, QIcon::Off),
        QIcon::Selected, QIcon::Off);
    icon.addPixmap(icon.pixmap(icon_size, QIcon::Normal, QIcon::On),
        QIcon::Selected, QIcon::On);
    return icon;
}

QIcon ThemeManager::GetPlayingIcon() const {
    QVariantMap font_options;
    font_options.insert(FontIconOption::colorAttr, QColor(252, 215, 75));
    return qFontIcon.GetIcon(0xF8F2, font_options);
}

QIcon ThemeManager::GetHiResIcon() const {
    QVariantMap options;
    options.insert(FontIconOption::colorAttr, QColor(250, 197, 24));
    options.insert(FontIconOption::scaleFactorAttr, 0.8);
    return QIcon(qFontIcon.GetIcon(0xE1AE, options));
}

QPixmap ThemeManager::GetGithubIcon() const {
    if (GetThemeColor() == ThemeColor::DARK_THEME) {
        return QPixmap(qTEXT(":/xamp/Resource/Black/GitHub-Mark.png"));
	} else {
        return QPixmap(qTEXT(":/xamp/Resource/White/GitHub-Mark.png"));
	}
}

const QPixmap& ThemeManager::GetUnknownCover() const noexcept {
    return unknown_cover_;
}

const QPixmap& ThemeManager::DefaultSizeUnknownCover() const noexcept {
    return default_size_unknown_cover_;
}

void ThemeManager::UpdateMaximumIcon(QToolButton *maxWinButton, bool is_maximum) const {
    if (is_maximum) {
        maxWinButton->setIcon(GetFontIcon(Glyphs::ICON_RESTORE_WINDOW));        
    } else {
        maxWinButton->setIcon(GetFontIcon(Glyphs::ICON_MAXIMUM_WINDOW));
    }
}

void ThemeManager::SetBitPerfectButton(QToolButton * bitPerfectButton, bool enable) {
    auto f = bitPerfectButton->font();
    f.setPointSize(GetFontSize(9));
    bitPerfectButton->setFont(f);
    bitPerfectButton->setText(qSTR("Bit-Perfect"));

    if (enable) {        
        bitPerfectButton->setStyleSheet(qSTR(
            R"(
                QToolButton#bitPerfectButton {
					font-family: "FormatFont";
					font-weight: bold;
					color: white;
                    border: none;
                    background-color: rgb(167, 200, 255);
                }
            )"
        ));
    }
    else {        
        bitPerfectButton->setStyleSheet(qSTR(
            R"(
                QToolButton#bitPerfectButton {
					font-family: "FormatFont";
					font-weight: bold;
					color: white;
                    border: none;
                    background-color: rgba(167, 200, 255, 60);
                }
            )"
        ));
    }
}

void ThemeManager::SetHeartButton(QToolButton* heartButton, bool press) {
    heartButton->setIcon(GetFontIcon(press ? Glyphs::ICON_HEART_PRESS : Glyphs::ICON_HEART));
    heartButton->setStyleSheet(qTEXT("background: transparent;"));
}

void ThemeManager::SetPlayOrPauseButton(QToolButton *playButton, bool is_playing) {
    if (is_playing) {
        playButton->setIcon(GetFontIcon(Glyphs::ICON_PAUSE));
    }
    else {
        playButton->setIcon(GetFontIcon(Glyphs::ICON_PLAY));
    }
}

const QSize& ThemeManager::GetDefaultCoverSize() const noexcept {
    return cover_size_;
}

QSize ThemeManager::GetCacheCoverSize() const noexcept {
    return cache_cover_size_;
}

QSize ThemeManager::GetAlbumCoverSize() const noexcept {
    return album_cover_size_;
}

void ThemeManager::SetBackgroundColor(QWidget* widget) {
    auto color = GetPalette().color(QPalette::WindowText);
    widget->setStyleSheet(BackgroundColorToString(color));
}

void ThemeManager::LoadAndApplyTheme() {
    qApp->setFont(GetDefaultFont());

    QString filename;

    if (GetThemeColor() == ThemeColor::LIGHT_THEME) {
        filename = qTEXT(":/xamp/Resource/Theme/light/style.qss");
    } else {
        filename = qTEXT(":/xamp/Resource/Theme/dark/style.qss");
    }

    QFile f(filename);
    f.open(QFile::ReadOnly | QFile::Text);
    QTextStream ts(&f);
    ts.setEncoding(QStringConverter::Utf8);
    qApp->setStyleSheet(ts.readAll());
    f.close();
}

void ThemeManager::SetBackgroundColor(QColor color) {
    background_color_ = color;
    qAppSettings.SetValue(kAppSettingBackgroundColor, color);
}

QColor ThemeManager::GetTitleBarColor() const {
    return QColor(GetBackgroundColor());
}

QColor ThemeManager::GetCoverShadowColor() const {
    switch (GetThemeColor()) {
    case ThemeColor::DARK_THEME:
    case ThemeColor::LIGHT_THEME:
    default:
        return {qTEXT("#DCDCDC")};
    }
}

QString ThemeManager::GetLinearGradientStyle() const {
    switch (GetThemeColor()) {
    default:
    case ThemeColor::DARK_THEME:
        return qTEXT("#2e2f31");
    case ThemeColor::LIGHT_THEME:
        return qTEXT("#ffffff");
    }
}

void ThemeManager::SetLinearGradient(QLinearGradient& gradient) const {
    switch (GetThemeColor()) {
        case ThemeColor::DARK_THEME:
            gradient.setColorAt(0, QColor("#1e1d23"));
            gradient.setColorAt(0.74, QColor("#000000"));
            break;
        case ThemeColor::LIGHT_THEME:       
            gradient.setCoordinateMode(QGradient::StretchToDeviceMode);
            gradient.setStart(0, 0);
            gradient.setFinalStop(1, 1);
            gradient.setColorAt(0, QColor(Qt::white));
            gradient.setColorAt(1, QColor(Qt::lightGray));
            break;
    }
}

QSize ThemeManager::GetTabIconSize() const {
#ifdef XAMP_OS_MAC
    return QSize(20, 20);
#else
    return QSize(18, 18);
#endif
}

QColor ThemeManager::GetHoverColor() const {
    switch (GetThemeColor()) {
    case ThemeColor::DARK_THEME:
        return {qTEXT("#43474e")};
    case ThemeColor::LIGHT_THEME:
    default:
        return {qTEXT("#C9CDD0")};
    }
}

QColor ThemeManager::GetHighlightColor() const {
    switch (GetThemeColor()) {
    case ThemeColor::LIGHT_THEME:
        return {qTEXT("#9FCBFF")};
    case ThemeColor::DARK_THEME:
    default:
        return {qTEXT("#1A72BB")};
    }
}

int32_t ThemeManager::GetTitleBarIconHeight() {
    return 8;
}

void ThemeManager::SetTitleBarButtonStyle(QToolButton* close_button, QToolButton* min_win_button, QToolButton* max_win_button) const {
    const QColor hover_color = GetHoverColor();
    const QColor color_hover_color(qTEXT("#dc3545"));

    close_button->setStyleSheet(qSTR(R"(
                                         QToolButton#closeButton {
                                         border: none;
                                         background-color: transparent;
										 border-radius: 0px;
                                         }

										 QToolButton#closeButton:hover {
										 background-color: %1;
										 border-radius: 0px;
                                         }
                                         )").arg(ColorToString(color_hover_color)));
    close_button->setIconSize(QSize(GetTitleBarIconHeight(), GetTitleBarIconHeight()));
    close_button->setIcon(GetFontIcon(Glyphs::ICON_CLOSE_WINDOW));

    min_win_button->setStyleSheet(qSTR(R"(
                                          QToolButton#minWinButton {
                                          border: none;
                                          background-color: transparent;
										  border-radius: 0px;
                                          }
										  QToolButton#minWinButton:hover {		
										  background-color: %1;
										  border-radius: 0px;				 
                                          }
                                          )").arg(ColorToString(hover_color)));
    min_win_button->setIconSize(QSize(GetTitleBarIconHeight(), GetTitleBarIconHeight()));
    min_win_button->setIcon(GetFontIcon(Glyphs::ICON_MINIMIZE_WINDOW));
    
    max_win_button->setStyleSheet(qSTR(R"(
                                          QToolButton#maxWinButton {
                                          border: none;
                                          background-color: transparent;
										  border-radius: 0px;
                                          }
										  QToolButton#maxWinButton:hover {		
										  background-color: %1;
										  border-radius: 0px;								 
                                          }
                                          )").arg(ColorToString(hover_color)));
    max_win_button->setIconSize(QSize(GetTitleBarIconHeight(), GetTitleBarIconHeight()));
    max_win_button->setIcon(GetFontIcon(Glyphs::ICON_MAXIMUM_WINDOW));
}

void ThemeManager::SetTextSeparator(QFrame *frame) {
    switch (GetThemeColor()) {
    case ThemeColor::DARK_THEME:
        frame->setStyleSheet(qTEXT("background-color: #37414F;"));
        break;
    case ThemeColor::LIGHT_THEME:
        frame->setStyleSheet(qTEXT("background-color: #CED1D4;"));
        break;
    }
}

int32_t ThemeManager::GetFontSize(int32_t base_size) const {
    return base_size * font_ratio_;
}

int32_t ThemeManager::GetDefaultFontSize() const {
    return GetFontSize(9);
}

QSize ThemeManager::GetTitleButtonIconSize() {
    return {GetTitleBarIconHeight(), GetTitleBarIconHeight()};
}

QFont ThemeManager::GetFormatFont() const {
    return QFont(qTEXT("FormatFont"));
}

QFont ThemeManager::GetUiFont() const {
    return QFont(qTEXT("UIFont"));
}

QFont ThemeManager::GetDisplayFont() const {
    return QFont(qTEXT("DisplayFont"));
}

QFont ThemeManager::GetMonoFont() const {
    return QFont(qTEXT("MonoFont"));
}

QFont ThemeManager::GetDebugFont() const {
    return QFont(qTEXT("DebugFont"));
}

void ThemeManager::SetComboBoxStyle(QComboBox* combo_box, const QString& object_name) {
    QString border_color;
    QString selection_background_color;
    QString on_selection_background_color;

    switch (GetThemeColor()) {
    case ThemeColor::LIGHT_THEME:
        border_color = "#C9CDD0";
        selection_background_color = "#FAFAFA";
        on_selection_background_color = "#1e1d23";
        break;
    case ThemeColor::DARK_THEME:
        border_color = "#455364";
        selection_background_color = "#1e1d23";
        on_selection_background_color = "#9FCBFF";
        break;
    }

    combo_box->setStyleSheet(qSTR(R"(
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

void ThemeManager::SetLineEditStyle(QLineEdit* line_edit, const QString& object_name) {
    switch (GetThemeColor()) {
        case ThemeColor::DARK_THEME:
            line_edit->setStyleSheet(qSTR(R"(
                                            QLineEdit#%1 {
                                            background-color: %2;
                                            border: 1px solid #4d4d4d;
                                            color: white;
                                            border-radius: 8px;
                                            }
                                            )").arg(object_name).arg("#3a3a3c"));
			break;
        case ThemeColor::LIGHT_THEME:
            line_edit->setStyleSheet(qSTR(R"(
                                            QLineEdit#%1 {
                                            background-color: %2;
                                            border: 1px solid gray;
                                            color: black;
                                            border-radius: 8px;
                                            }
                                            )").arg(object_name).arg(ColorToString(Qt::white)));
            break;
    }
}

void ThemeManager::SetMuted(QAbstractButton *button, bool is_muted) {
    if (!is_muted) {
        button->setIcon(GetFontIcon(Glyphs::ICON_VOLUME_UP));
        qAppSettings.SetValue<bool>(kAppSettingIsMuted, false);
    }
    else {
        button->setIcon(GetFontIcon(Glyphs::ICON_VOLUME_OFF));
        qAppSettings.SetValue(kAppSettingIsMuted, true);
    }
}

void ThemeManager::SetVolume(QSlider *slider, QAbstractButton* button, uint32_t volume) {
    if (!slider->isEnabled()) {
        return;
    }
    if (volume == 0) {
        SetMuted(button, true);
    }
    else {
        SetMuted(button, false);
    }
    slider->setValue(volume);
}

Glyphs ThemeManager::GetConnectTypeGlyphs(DeviceConnectType type) const {
    switch (type) {
    case DeviceConnectType::CONNECT_TYPE_UNKNOWN:
    case DeviceConnectType::CONNECT_TYPE_BUILT_IN:
        return Glyphs::ICON_BUILD_IN_SPEAKER;
    case DeviceConnectType::CONNECT_TYPE_USB:
        return Glyphs::ICON_USB;
    case DeviceConnectType::CONNECT_TYPE_BLUE_TOOTH:
        return Glyphs::ICON_BLUE_TOOTH;
    }
    return Glyphs::ICON_BUILD_IN_SPEAKER;
}

QIcon ThemeManager::GetConnectTypeIcon(DeviceConnectType type) const {
    switch (type) {
    case DeviceConnectType::CONNECT_TYPE_UNKNOWN:
    case DeviceConnectType::CONNECT_TYPE_BUILT_IN:
        return GetFontIcon(Glyphs::ICON_BUILD_IN_SPEAKER);
    case DeviceConnectType::CONNECT_TYPE_USB:
        return GetFontIcon(Glyphs::ICON_USB);
    case DeviceConnectType::CONNECT_TYPE_BLUE_TOOTH:
        return GetFontIcon(Glyphs::ICON_BLUE_TOOTH);
    }
    return GetFontIcon(Glyphs::ICON_BUILD_IN_SPEAKER);
}

void ThemeManager::SetDeviceConnectTypeIcon(QAbstractButton* button, DeviceConnectType type) {
    button->setIcon(GetConnectTypeIcon(type));
    button->update();
}

void ThemeManager::SetSliderTheme(QSlider* slider, bool enter) {
    QString slider_background_color;
    QString slider_border_color;

	switch (GetThemeColor()) {
	case ThemeColor::LIGHT_THEME:
       slider_background_color = qTEXT("#9FCBFF");
       slider_border_color = qTEXT("#C9CDD0");
       break;
	case ThemeColor::DARK_THEME:
       slider_background_color = qTEXT("#1A72BB");
       slider_border_color = qTEXT("#43474e");
       break;
   }

    auto handle_border_color = slider_background_color;
    auto margin = 10;
    if (!enter) {
        handle_border_color = qTEXT("transparent");
        margin = 1;
    }

    slider->setStyleSheet(qTEXT(R"(
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
        width: 10px;
		height: 10px;
        margin: 0px -5px 0px -5px;
		border-radius: 5px;
		background-color: %2;
		border: 1px solid %2;
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
        margin: -5px 0px -5px 0px;
		border-radius: 5px;
		background-color: %4;
		border: 1px solid %4;
    }
    )"
    ).arg(slider->objectName())
        .arg(slider_background_color)
        .arg(slider_border_color)
		.arg(handle_border_color)
        .arg(margin)
    );
}

void ThemeManager::SetAlbumNaviBarTheme(QListView *tab) const {
    QString tab_left_color;

    switch (GetThemeColor()) {
    case ThemeColor::DARK_THEME:
        tab_left_color = qTEXT("42, 130, 218");
        break;
    case ThemeColor::LIGHT_THEME:
        tab_left_color = qTEXT("42, 130, 218");
        break;
    }

    tab->setStyleSheet(qSTR(R"(
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

