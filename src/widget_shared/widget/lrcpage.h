//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QPixmap>

#include <QTableWidget>
#include <QListWidget>
#include <QLabel>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

#include <widget/themecolor.h>
#include <widget/widget_shared_global.h>
#include <widget/krcparser.h>
#include <widget/worker/backgroundservice.h>

class QLabel;
class VinylWidget;
class ScrollLabel;
class LyricsShowWidget;
class SpectrumWidget;
class QPaintEvent;

enum {
	LRC_PAGE_TAB_ID = 0,
	LRC_PAGE_TAB_ALBUM,
	LRC_PAGE_TAB_ARTIST,
	LRC_PAGE_TAB_TITLE,
	LRC_PAGE_TAB_LYRICS,
	LRC_PAGE_TAB_KARAOKE,
	LRC_PAGE_TAB_DURATION,
};

class MultiColumnFilterModel : public QSortFilterProxyModel {
public:
	using QSortFilterProxyModel::QSortFilterProxyModel;

protected:
	bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override {
		// 取得目前的正則表示式
		QRegularExpression re = filterRegularExpression();
		// 如果尚未輸入搜尋字串，則顯示全部
		if (re.pattern().isEmpty()) {
			return true;
		}

		// 設定忽略大小寫（等效於 Qt::CaseInsensitive）
		// 也可以在 setFilterRegularExpression() 時就設定好 Options
		QRegularExpression::PatternOptions opts;
		opts |= QRegularExpression::CaseInsensitiveOption;
		re.setPatternOptions(opts);

		// 檢查目標欄位是否含有此正則
		std::array<uint32_t, 3> columns = { LRC_PAGE_TAB_ALBUM, LRC_PAGE_TAB_ARTIST, LRC_PAGE_TAB_TITLE };
		for (uint32_t col : columns) {
			QModelIndex index = sourceModel()->index(sourceRow, col, sourceParent);
			QString dataStr = index.data(Qt::DisplayRole).toString();
			// 只要有一欄匹配，就顯示
			if (dataStr.contains(re)) {
				return true;
			}
		}
		// 都沒有匹配則隱藏該列
		return false;
	}
};

class LyricsFrame : public QFrame {
	Q_OBJECT
public:
	explicit LyricsFrame(QWidget* parent = nullptr);

	void setLyrics(const QList<SearchLyricsResult>& results);

signals:
	void changeLyric(const LyricsParser &parser);

private:
	QTableView* lyrc_view_;
	MultiColumnFilterModel* filterModel_;
	QStandardItemModel* model_;
	QMap<QString, LyricsParser> parser_map_;
};

class XAMP_WIDGET_SHARED_EXPORT LrcPage : public QFrame {
	Q_OBJECT
	Q_PROPERTY(int disappearBgProg READ getDisappearBgProgress WRITE setDisappearBgProgress)
	Q_PROPERTY(int appearBgProg READ getAppearBgProgress WRITE setAppearBgProgress)

public:
	static constexpr auto kBlurAlpha = 200;
	static constexpr int kBlurBackgroundAnimationMs = 1500;

	explicit LrcPage(QWidget* parent = nullptr);

	QSize coverSizeHint() const;

	LyricsShowWidget* lyrics();

	void setCover(const QPixmap& cover);

	void setPlayListEntity(const PlayListEntity& entity);

	void addCoverShadow(bool is_dark = false);

	QLabel* format();

	ScrollLabel* album();

	ScrollLabel* artist();

	ScrollLabel* title();

	QLabel* cover();

	QSize coverSize() const;

	SpectrumWidget* spectrum();

	void setFullScreen();

	void disableLoadLrcButton();

public slots:
	void onThemeChangedFinished(ThemeColor theme_color);

	void setBackground(const QImage& cover);

	void clearBackground();

	void onFetchLyricsCompleted(const QList<SearchLyricsResult>& results);

private:
	void setAppearBgProgress(int x);

	int getAppearBgProgress() const;

	void setDisappearBgProgress(int x);

	int getDisappearBgProgress() const;

	void startBackgroundAnimation(int durationMs);

	void paintEvent(QPaintEvent*) override;

	void resizeEvent(QResizeEvent* event) override;

	void initial();

	int current_bg_alpha_ = 255;
	int prev_bg_alpha_ = 0;
	LyricsShowWidget* lyrics_widget_;
	QLabel* cover_label_;
	QLabel* format_label_;
	ScrollLabel* album_;
	ScrollLabel* artist_;
	ScrollLabel* title_;
	SpectrumWidget* spectrum_;
	QToolButton* change_lrc_button_;
	QToolButton* change_color_button_;
	QToolButton* change_highlight_button_;
	QToolButton* karaoke_highlight_button_;
	QPixmap cover_;
	QImage background_image_;
	QImage prev_background_image_;
	PlayListEntity entity_;
	QList<SearchLyricsResult> lyrics_results_;
};
