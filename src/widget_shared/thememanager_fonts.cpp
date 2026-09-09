//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <thememanager.h>

#include <widget/util/json_util.h>
#include <widget/util/str_util.h>
#include <base/logger.h>

#include <QCoreApplication>
#include <QDirIterator>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QList>
#include <QMap>
#include <QString>
#include <QVariant>

#include <algorithm>

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
    if (font_families.isEmpty()) {
        XAMP_LOG_ERROR("load font failed: {}", file_name.toStdString());
        return;
    }

    for (const auto& family : font_families) {
        if (!ui_fallback_fonts.contains(family)) {
            ui_fallback_fonts.push_back(family);
        }
    }
}

void ThemeManager::setGoogleMaterialFontIcons() {
    HashMap<int32_t, uint32_t> glyphs_lut;

    QJsonDocument doc;
    const auto font_icon_json_path = qFormat("%1/fonticon.json")
        .arg(QCoreApplication::applicationDirPath());
    const QFileInfo font_icon_json_info(font_icon_json_path);
    XAMP_LOG_DEBUG("load font icon glyph config: {} exists:{}",
        font_icon_json_path.toStdString(),
        font_icon_json_info.exists());

    if (font_icon_json_info.exists() && json_util::deserializeFile(font_icon_json_path, doc)) {
        auto jsonObject = doc.object();
        for (auto it = jsonObject.begin(); it != jsonObject.end(); ++it) {
            auto key = it.key().toStdString();
            auto value = it.value().toString();
            Glyphs glyph;
            if (FindGlyphs(key, glyph)) {
				glyphs_lut.insert(std::make_pair(static_cast<int32_t>(glyph), value.toUInt(nullptr, 16)));
            }
        }
    } else {
        XAMP_LOG_ERROR("load font icon glyph config failed: {}", font_icon_json_path.toStdString());
    }

    const auto material_icon_font_path = fontNamePath("MaterialIcons-Regular.ttf"_str);
    const QFileInfo material_icon_font_info(material_icon_font_path);
    XAMP_LOG_DEBUG("load Material Icons font: {} exists:{}",
        material_icon_font_path.toStdString(),
        material_icon_font_info.exists());

    const auto material_icon_loaded = qFontIcon.addFont(material_icon_font_path);
    XAMP_LOG_DEBUG("Material Icons font loaded:{} families:{} glyphs:{}",
        material_icon_loaded,
        qFontIcon.getFamilies().join(","_str).toStdString(),
        glyphs_lut.size());
    qFontIcon.setGlyphs(glyphs_lut);
}

QFont ThemeManager::loadFonts() {
    QList<QString> format_font;
    QList<QString> mono_fonts;
    QList<QString> display_fonts;
    QList<QString> ui_fonts;
	QList<QString> source_han_fonts;
    QList<QString> en_fonts;
    QList<QString> debug_fonts;

    installFileFonts("IBMPlexMono-Regular"_str, debug_fonts);
    installFileFonts("Aldrich-Regular"_str, format_font);
    installFileFonts("NotoSans"_str, mono_fonts);

    installFileFonts("SourceHanSans"_str, source_han_fonts);
    source_han_fonts.push_back("Source Han Sans TC"_str);
    source_han_fonts.push_back("Source Han Sans JP"_str);
    source_han_fonts.push_back("Source Han Sans SC"_str);
    sortFontWeight(source_han_fonts.begin(), source_han_fonts.end());

    ui_fonts.append(source_han_fonts);

    if (format_font.isEmpty()) {
        format_font = mono_fonts;
    }
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
#ifdef Q_OS_WIN
    ui_font.setFamilies({QStringLiteral("Segoe UI"), QStringLiteral("Microsoft JhengHei UI"), QStringLiteral("Yu Gothic UI")});
#endif
    ui_font.setStyleStrategy(QFont::PreferAntialias);
    ui_font.setWeight(QFont::Weight::Normal);
    ui_font.setKerning(true);

    return ui_font;
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

