#include <qDebug>
#include <QToolTip>
#include <QFontDatabase>
#include <QMenu>
#include <QCloseEvent>
#include <QFileInfo>
#include <QDesktopWidget>

#include "widget/lrcpage.h"
#include "widget/win32/blur_effect_helper.h"
#include "widget/albumview.h"
#include "widget/str_utilts.h"
#include "widget/playlistpage.h"
#include "widget/toast.h"
#include "widget/image_utiltis.h"
#include "widget/time_utilts.h"
#include "widget/database.h"
#include "widget/pixmapcache.h"

#include "xamp.h"

static QString colorToRgba(const QColor& c) {
	return QString(Q_UTF8("rgba(%1, %2, %3, %4)"))
		.arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha());
}

static QString getUIFormat(const AudioFormat& format, const PlayListEntity& item) {
	auto ext = item.file_ext;
	ext = ext.remove(Q_UTF8(".")).toUpper();
	auto is_mhz_samplerate = false;
	auto precision = 1;
	if (format.GetSampleRate() / 1000 > 1000) {
		is_mhz_samplerate = true;
	}
	else {
		precision = format.GetSampleRate() % 1000 == 0 ? 0 : 1;
	}
	auto bits = format.GetBitsPerSample();
	if (is_mhz_samplerate) {
		bits = 1;
	}
	return ext
		+ Q_UTF8(" | ")
		+ (is_mhz_samplerate ? QString::number(format.GetSampleRate() / double(1000000), 'f', 2) + Q_UTF8("MHz/")
		: QString::number(format.GetSampleRate() / double(1000), 'f', precision) + Q_UTF8("kHz/"))
		+ QString::number(bits) + Q_UTF8("bit");
}

Xamp::Xamp(QWidget *parent)
	: FramelessWindow(parent)
	, is_seeking_(false)
	, order_(PlayerOrder::PLAYER_ORDER_REPEAT_ONCE)
	, lrc_page_(nullptr)
	, state_adapter_(std::make_shared<PlayerStateAdapter>())
	, player_(std::make_shared<AudioPlayer>(state_adapter_)) {
	initialUI();	
	initialController();
	initialDeviceList();
	initialPlaylist();	
	setCover(QPixmap(Q_UTF8(":/xamp/Resource/White/unknown_album.png")));
	//setCentralWidget(ui.centralWidget); // For QMainWindow use
}

void Xamp::closeEvent(QCloseEvent* event) {
	AppSettings::settings().setValue(APP_SETTING_WIDTH, size().width());
	AppSettings::settings().setValue(APP_SETTING_HEIGHT, size().height());

	if (player_ != nullptr) {
		player_->Stop(false, true);
		player_.reset();
	}
}

void Xamp::setNightMode() {
}

void Xamp::setNormalMode() {
	ui.currentView->setStyleSheet(Q_UTF8("background-color: rgba(228, 233, 237, 230);"));
	ui.titleFrame->setStyleSheet(Q_UTF8("background-color: rgba(228, 233, 237, 230);"));
	ui.sliderBar->setStyleSheet(Q_UTF8("background-color: rgba(228, 233, 237, 150);"));
	ui.controlFrame->setStyleSheet(Q_UTF8("background-color: rgba(255, 255, 255, 200);"));
	ui.volumeFrame->setStyleSheet(Q_UTF8("background-color: rgba(255, 255, 255, 200);"));
	ui.playingFrame->setStyleSheet(Q_UTF8("background-color: rgba(228, 233, 237, 220);"));
	ui.searchLineEdit->setStyleSheet(Q_UTF8("background: transparent;"));

	setStyleSheet(Q_UTF8(R"(
	QTableView {
		background-color: transparent;
		color: black;
	}
	)"));

	ui.closeButton->setStyleSheet(Q_UTF8(R"(
		QToolButton#closeButton { 
			image: url(:/xamp/Resource/White/close.png);
			background: transparent;
			background-color: rgba(255, 255, 255, 0);
		}
	)"));

	ui.minWinButton->setStyleSheet(Q_UTF8(R"(
		QToolButton#minWinButton { 
			image: url(:/xamp/Resource/White/minimize.png);
			background: transparent; 
			background-color: rgba(255, 255, 255, 0);
		}
	)"));

	ui.maxWinButton->setStyleSheet(Q_UTF8(R"(
		QToolButton#maxWinButton { 
			image: url(:/xamp/Resource/White/maximize.png);
			background: transparent;
			background-color: rgba(255, 255, 255, 0);
		}
	)"));

	ui.settingsButton->setStyleSheet(Q_UTF8(R"(
		QToolButton#settingsButton { 
			image: url(:/xamp/Resource/White/settings.png);
			background: transparent;
			background-color: rgba(255, 255, 255, 0);
		}
	)"));

	setPlayOrPauseButton(false);

	ui.nextButton->setStyleSheet(Q_UTF8(R"(
		QToolButton#nextButton { 
			image: url(:/xamp/Resource/White/next.png);
			background: transparent;
		 }
	)"));

	ui.prevButton->setStyleSheet(Q_UTF8(R"(
		QToolButton#prevButton {
			image: url(:/xamp/Resource/White/previous.png);
			background: transparent;
		 }
	)"));

	ui.nextPageButton->setStyleSheet(Q_UTF8(R"(
		QToolButton#nextPageButton {
			image: url(:/xamp/Resource/White/right_black.png);
			background: transparent;
		}
	)"));

	ui.backPageButton->setStyleSheet(Q_UTF8(R"(
		QToolButton#backPageButton {
			image: url(:/xamp/Resource/White/left_black.png);
			background: transparent;
		}
	)"));

	ui.searchLineEdit->setStyleSheet(Q_UTF8(R"(
		QLineEdit#searchLineEdit {
			background-color: white;
			border: none;
		}
	)"));

	ui.mutedButton->setStyleSheet(Q_UTF8(R"(
		QToolButton#mutedButton {
			image: url(:/xamp/Resource/White/volume_up.png);
			background: transparent;
		}
	)"));

	ui.selectDeviceButton->setStyleSheet(Q_UTF8(R"(
		QToolButton#selectDeviceButton {
			image: url(:/xamp/Resource/White/speaker.png);
			background: transparent;
		}
	)"));

	ui.playlistButton->setStyleSheet(Q_UTF8(R"(
		QToolButton#playlistButton {
			background: transparent;
		}
	)"));
	ui.volumeSlider->setStyleSheet(Q_UTF8(R"(
		QSlider#volumeSlider {
			background: transparent;
		}
	)"));

	ui.addPlaylistButton->setStyleSheet(Q_UTF8(R"(
		QToolButton#addPlaylistButton {
			background: transparent;
		}
	)"));

	ui.repeatButton->setStyleSheet(Q_UTF8(R"(
		QToolButton#repeatButton {
			image: url(:/xamp/Resource/White/repeat.png);
			background: transparent;
		}
	)"));

	ui.addPlaylistButton->setStyleSheet(Q_UTF8(R"(
		QToolButton#addPlaylistButton {
			image: url(:/xamp/Resource/White/create_new_folder.png);
			background: transparent;
		}
	)"));

	ui.titleLabel->setStyleSheet(Q_UTF8(R"(
		QLabel#titleLabel {
			background: transparent;
		}
	)"));

	ui.artistLabel->setStyleSheet(Q_UTF8(R"(
		QLabel#artistLabel {
			background: transparent;
		}
	)"));

	ui.startPosLabel->setStyleSheet(Q_UTF8(R"(
		QLabel#startPosLabel {
			background: transparent;
			color: gray;
		}
	)"));

	ui.endPosLabel->setStyleSheet(Q_UTF8(R"(
		QLabel#endPosLabel {
			background: transparent;
			color: gray;
		}
	)"));

	ui.seekSlider->setStyleSheet(Q_UTF8(R"(
		QSlider#seekSlider {
			background: transparent;
		}
	)"));

	ui.artistLabel->setStyleSheet(Q_UTF8(R"(
		QLabel#artistLabel {
			background: transparent;
			color: gray;
		}
	)"));
}

void Xamp::initialUI() {
	ui.setupUi(this);
	setNormalMode();
	setGeometry(
		QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter,
			AppSettings::settings().getSizeValue(APP_SETTING_WIDTH, APP_SETTING_HEIGHT),
			qApp->desktop()->availableGeometry()
		)
	);
}

void Xamp::setPlayOrPauseButton(bool is_playing) {
	if (is_playing) {
		ui.playButton->setStyleSheet(Q_UTF8(R"(
		QToolButton#playButton {
			image: url(:/xamp/Resource/White/pause.png);
			background: transparent;
		}
		)"));
	}
	else {
		ui.playButton->setStyleSheet(Q_UTF8(R"(
		QToolButton#playButton {
			image: url(:/xamp/Resource/White/play.png);
			background: transparent;
		}
		)"));
	}
}

QWidgetAction* Xamp::createTextSeparator(const QString& text) {
	auto label = new QLabel(text);
	label->setObjectName(Q_UTF8("textSeparator"));
	auto f = font();
	f.setBold(true);
	label->setFont(f);
	auto separator = new QWidgetAction(this);
	separator->setDefaultWidget(label);
	return separator;
}

void Xamp::initialDeviceList() {
	auto menu = ui.selectDeviceButton->menu();
	if (!menu) {
		menu = new QMenu(this);
		ui.selectDeviceButton->setMenu(menu);
	}

	menu->setStyleSheet(Q_UTF8(R"(		
		QMenu {
			background-color: rgba(228, 233, 237, 150);
		}
		QMenu::item:selected { 
			background-color: black;
		}
		)"));
	menu->clear();

	DeviceInfo init_device_info;
	auto is_find_setting_device = false;

	auto device_action_group = new QActionGroup(this);
	device_action_group->setExclusive(true);

	std::map<std::wstring, QAction*> device_id_action;

	DeviceFactory::Instance().ForEach([&](const auto &device_type) {
		device_type->ScanNewDevice();

		const auto device_info_list = device_type->GetDeviceInfo();
		if (device_info_list.empty()) {
			return;
		}

		menu->addAction(createTextSeparator(QString::fromStdWString(device_type->GetName())));

		for (const auto& device_info : device_info_list) {
			auto device_action = new QAction(QString::fromStdWString(device_info.name), this);
			device_action_group->addAction(device_action);
			device_action->setCheckable(true);
			device_id_action[device_info.device_id] = device_action;
			(void)QObject::connect(device_action, &QAction::triggered, [device_info, this]() {
				device_info_ = device_info;
				AppSettings::settings().setValue(APP_SETTING_DEVICE_TYPE, QString::fromStdString(device_info_.device_type_id));
				AppSettings::settings().setValue(APP_SETTING_DEVICE_ID, QString::fromStdWString(device_info_.device_id));
				});
			menu->addAction(device_action);
			if (AppSettings::settings().getIDValue(APP_SETTING_DEVICE_TYPE) == device_info.device_type_id
				&& AppSettings::settings().getValue(APP_SETTING_DEVICE_ID).toString().toStdWString() == device_info.device_id) {
				device_info_ = device_info;
				is_find_setting_device = true;
				device_action->setChecked(true);
			}
		}

		if (!is_find_setting_device) {
			auto itr = std::find_if(device_info_list.begin(), device_info_list.end(), [](const auto& info) {
				return info.is_default_device && !DeviceFactory::IsExclusiveDevice(info);
				});
			if (itr != device_info_list.end()) {			
				init_device_info = (*itr);				
			}
		}
		});

	if (!is_find_setting_device) {
		device_info_ = init_device_info;
		device_id_action[device_info_.device_id]->setChecked(true);
		AppSettings::settings().setValue(APP_SETTING_DEVICE_TYPE, QString::fromStdString(device_info_.device_type_id));
		AppSettings::settings().setValue(APP_SETTING_DEVICE_ID, QString::fromStdWString(device_info_.device_id));
	}
}

void Xamp::initialController() {
	qRegisterMetaType<xamp::player::PlayerState>("xamp::player::PlayerState");
	qRegisterMetaType<xamp::base::Errors>("xamp::base::Errors");

	(void)QObject::connect(ui.minWinButton, &QToolButton::pressed, [this]() {
		showMinimized();
		});

	(void)QObject::connect(ui.maxWinButton, &QToolButton::pressed, [this]() {
		if (isMaximized()) {
			showNormal();
		}
		else {
			showMaximized();
		}
		});

	(void)QObject::connect(ui.closeButton, &QToolButton::pressed, [this]() {
		QWidget::close();
		});

	(void)QObject::connect(ui.settingsButton, &QToolButton::pressed, [this]() {

		});

	(void)QObject::connect(ui.seekSlider, &QSlider::sliderMoved, [this](auto value) {
		QToolTip::showText(QCursor::pos(), Time::msToString(double(ui.seekSlider->value()) / 1000.0));
		if (!is_seeking_) {
			return;
		}
		ui.seekSlider->setValue(value);
		XAMP_LOG_DEBUG("seeking {} sec.", value / 1000.0);
		});

	(void)QObject::connect(ui.seekSlider, &QSlider::sliderReleased, [this]() {
		QToolTip::showText(QCursor::pos(), Time::msToString(double(ui.seekSlider->value()) / 1000.0));
		if (!is_seeking_) {
			return;
		}
		XAMP_LOG_DEBUG("final seeking {} sec.", ui.seekSlider->value() / 1000.0);
		try {
			player_->Seek(static_cast<double>(ui.seekSlider->value() / 1000.0));
		}
		catch (const xamp::base::Exception& e) {
			player_->Stop(false);
			Toast::showTip(Q_UTF8(e.GetErrorMessage()), this);
			return;
		}		
		is_seeking_ = false;
		});

	(void)QObject::connect(ui.seekSlider, &QSlider::sliderPressed, [this]() {
		QToolTip::showText(QCursor::pos(), Time::msToString(double(ui.seekSlider->value()) / 1000.0));
		if (is_seeking_) {
			return;
		}
		is_seeking_ = true;
		player_->Pause();
		});

	order_ = static_cast<PlayerOrder>(AppSettings::settings().getValue(APP_SETTING_ORDER).toInt());
	setPlayerOrder();

	ui.volumeSlider->setRange(0, 100);
	ui.volumeSlider->setValue(AppSettings::settings().getValue(APP_SETTING_VOLUME).toInt());

	(void)QObject::connect(ui.volumeSlider, &QSlider::valueChanged, [this](auto volume) {
		QToolTip::showText(QCursor::pos(), tr("Volume : ") + QString::number(volume) + Q_UTF8("%"));
		setVolume(volume);
		AppSettings::settings().setValue(APP_SETTING_VOLUME, volume);
		});

	(void)QObject::connect(ui.volumeSlider, &QSlider::sliderMoved, [this](auto volume) {
		QToolTip::showText(QCursor::pos(), tr("Volume : ") + QString::number(volume) + Q_UTF8("%"));
		});

	(void)QObject::connect(state_adapter_.get(),
		&PlayerStateAdapter::stateChanged,
		this,
		&Xamp::onPlayerStateChanged,
		Qt::QueuedConnection);

	(void)QObject::connect(state_adapter_.get(),
		&PlayerStateAdapter::sampleTimeChanged,
		this,
		&Xamp::onSampleTimeChanged,
		Qt::QueuedConnection);

	(void)QObject::connect(ui.searchLineEdit, &QLineEdit::textChanged, [this](const auto &text) {
		if (ui.currentView->count() > 0) {
			auto playlist_view = playlist_page_->playlist();
			emit playlist_view->search(text, Qt::CaseSensitivity::CaseInsensitive, QRegExp::PatternSyntax());
		}
		});

	(void)QObject::connect(ui.nextButton, &QToolButton::pressed, [this]() {
		playNextClicked();
		});

	(void)QObject::connect(ui.prevButton, &QToolButton::pressed, [this]() {
		playPreviousClicked();
		});

	(void)QObject::connect(ui.repeatButton, &QToolButton::pressed, [this]() {
		order_ = static_cast<PlayerOrder>((order_ + 1) % PlayerOrder::_MAX_PLAYER_ORDER_);
		setPlayerOrder();
		});

	(void)QObject::connect(ui.playButton, &QToolButton::pressed, [this]() {
		play();
		});

	(void)QObject::connect(ui.backPageButton, &QToolButton::pressed, [this]() {
		goBackPage();
		});

	(void)QObject::connect(ui.nextPageButton, &QToolButton::pressed, [this]() {
		getNextPage();
		});


	(void)QObject::connect(ui.coverLabel, &ClickableLabel::clicked, [this]() {
		ui.currentView->setCurrentWidget(lrc_page_);
		});

	auto settings_menu = new QMenu(this);
	settings_menu->setStyleSheet(Q_UTF8(R"(		
		QMenu {
			background-color: rgba(228, 233, 237, 150);
		}
		QMenu::item:selected { 
			background-color: black;
		}
		)"));
	auto settings_action = new QAction(tr("Settings"), this);
	settings_menu->addAction(settings_action);
	auto night_mode_action = new QAction(tr("Night mode"), this);
	settings_menu->addAction(night_mode_action);
	settings_menu->addSeparator();
	auto about_action = new QAction(tr("About"), this);
	settings_menu->addAction(about_action);
	ui.settingsButton->setMenu(settings_menu);

	ui.seekSlider->setEnabled(false);
	ui.startPosLabel->setText(Time::msToString(0));
	ui.endPosLabel->setText(Time::msToString(0));
}

void Xamp::getNextPage() {
	if (stack_page_id_.isEmpty()) {
		return;
	}
	auto idx = ui.currentView->currentIndex();
	ui.currentView->setCurrentIndex(idx + 1);
}

void Xamp::goBackPage() {
	if (stack_page_id_.isEmpty()) {
		return;
	}
	auto idx = ui.currentView->currentIndex();
	ui.currentView->setCurrentIndex(idx - 1);
}

void Xamp::setVolume(int32_t volume) {
	if (volume > 0) {
		player_->SetMute(false);
		ui.mutedButton->setIcon(QIcon(Q_UTF8(":/xamp/Resource/White/volume_up.png")));
	}
	else {
		player_->SetMute(true);
		ui.mutedButton->setIcon(QIcon(Q_UTF8(":/xamp/Resource/White/volume_off.png")));
	}
	try {
		player_->SetVolume(volume);
	}
	catch (const xamp::base::Exception& e) {
		player_->Stop(false);
		Toast::showTip(Q_UTF8(e.GetErrorMessage()), this);
		return;
	}
}

void Xamp::initialShortcut() {

}

void Xamp::playNextClicked() {
	playNextItem(1);
}

void Xamp::playPreviousClicked() {
	playNextItem(-1);
}

void Xamp::onDeleteKeyPress() {
	if (!ui.currentView->count()) {
		return;
	}
	auto playlist_view = playlist_page_->playlist();
	playlist_view->removeSelectItems();
}

void Xamp::setPlayerOrder() {
	switch (order_) {
	case PlayerOrder::PLAYER_ORDER_REPEAT_ONCE:
		ui.repeatButton->setStyleSheet(Q_UTF8(R"(
		QToolButton#repeatButton {
			image: url(:/xamp/Resource/White/repeat.png);
			background: transparent;
		}
		)"));
		AppSettings::settings().setValue(APP_SETTING_ORDER, PLAYER_ORDER_REPEAT_ONCE);
		break;
	case PlayerOrder::PLAYER_ORDER_REPEAT_ONE:
		ui.repeatButton->setStyleSheet(Q_UTF8(R"(
		QToolButton#repeatButton {
			image: url(:/xamp/Resource/White/repeat_one.png);
			background: transparent;
		}
		)"));
		AppSettings::settings().setValue(APP_SETTING_ORDER, PLAYER_ORDER_REPEAT_ONE);
		break;
	case PlayerOrder::PLAYER_ORDER_SHUFFLE_ALL:
		ui.repeatButton->setStyleSheet(Q_UTF8(R"(
		QToolButton#repeatButton {
			image: url(:/xamp/Resource/White/shuffle.png);
			background: transparent;
		}
		)"));
		AppSettings::settings().setValue(APP_SETTING_ORDER, PlayerOrder::PLAYER_ORDER_SHUFFLE_ALL);
		break;
	}
}

void Xamp::playNextItem(int32_t forward) {
	auto playlist_view = playlist_page_->playlist();
	const auto count = playlist_view->model()->rowCount();
	if (count == 0) {
		stopPlayedClicked();
		return;
	}

	switch (order_) {
	case PlayerOrder::PLAYER_ORDER_REPEAT_ONE:
		play_index_ = playlist_view->currentIndex();
		break;
	case PlayerOrder::PLAYER_ORDER_REPEAT_ONCE:
		play_index_ = playlist_view->currentIndex();
		play_index_ = playlist_view->model()->index(play_index_.row() + forward, PLAYLIST_PLAYING);
		if (play_index_.row() == -1) {
			return;
		}
		break;
	case PlayerOrder::PLAYER_ORDER_SHUFFLE_ALL:
		if (count > 1) {
			play_index_ = playlist_view->shuffeIndex();
		}
		break;
	}

	if (!play_index_.isValid()) {
		Toast::showTip(tr("Not found any playlist item."), this);
		return;
	}
	playlist_view->setNowPlaying(play_index_, true);
	playlist_view->play(play_index_);
}

void Xamp::onSampleTimeChanged(double stream_time) {
	if (!player_) {
		return;
	}
	if (player_->GetState() == PlayerState::PLAYER_STATE_RUNNING) {
		ui.endPosLabel->setText(Time::msToString(player_->GetDuration() - stream_time));
		auto stream_time_as_ms = static_cast<int32_t>(stream_time * 1000.0);
		ui.seekSlider->setValue(stream_time_as_ms);
		ui.startPosLabel->setText(Time::msToString(stream_time));
		setTaskbarProgress(100.0 * ui.seekSlider->value() / ui.seekSlider->maximum());
		lrc_page_->lyricsWidget()->setLrcTime(stream_time_as_ms);
	}
}

void Xamp::playLocalFile(const PlayListEntity& item) {
	setTaskbarPlayerPlaying();
	play(item);
}

void Xamp::play() {
	if (player_->GetState() == xamp::player::PlayerState::PLAYER_STATE_RUNNING) {
		setPlayOrPauseButton(false);
		player_->Pause();
		setTaskbarPlayerPaused();
	}
	else if (player_->GetState() == xamp::player::PlayerState::PLAYER_STATE_PAUSED) {
		setPlayOrPauseButton(true);
		player_->Resume();
		setTaskbarPlayingResume();
	}
	else if (player_->GetState() == xamp::player::PlayerState::PLAYER_STATE_STOPPED) {
		if (!ui.currentView->count()) {
			return;
		}
		if (auto select_item = playlist_page_->playlist()->selectItem()) {
			play_index_ = select_item.value();
		}
		play_index_ = playlist_page_->playlist()->model()->index(
			play_index_.row(), PLAYLIST_PLAYING);
		if (play_index_.row() == -1) {
			Toast::showTip(tr("Not found any playing item."), this);
			return;
		}
		playlist_page_->playlist()->setNowPlaying(play_index_, true);
		playlist_page_->playlist()->play(play_index_);
	}
}

void Xamp::play(const PlayListEntity& item) {
	ui.titleLabel->setText(item.title);
	ui.artistLabel->setText(item.artist);

	auto use_bass_stream = QStringLiteral(".dsd,.dsf,.dff,.m4a").contains(item.file_ext);
	player_->Open(item.file_path.toStdWString(), use_bass_stream, device_info_);
	
	if (!player_->IsMute()) {		
		setVolume(ui.volumeSlider->value());
	} else {
		setVolume(0);
	}

	ui.seekSlider->setRange(0, int32_t(player_->GetDuration() * 1000));	
	ui.endPosLabel->setText(Time::msToString(player_->GetDuration()));

	player_->PlayStream();	
}

void Xamp::play(const QModelIndex& index, const PlayListEntity& item) {
	try {
		ui.seekSlider->setEnabled(true);
		playLocalFile(item);
		setPlayOrPauseButton(true);
		playlist_page_->format()->setText(getUIFormat(player_->GetStreamFormat(), item));
	} catch(const xamp::base::Exception& e) {
		ui.seekSlider->setEnabled(false);
		player_->Stop(false, true);
		Toast::showTip(Q_UTF8(e.GetErrorMessage()), this);
	} catch (const std::exception& e) {
		ui.seekSlider->setEnabled(false);
		player_->Stop(false, true);
		Toast::showTip(Q_UTF8(e.what()), this);
	} catch (...) {
		ui.seekSlider->setEnabled(false);
		player_->Stop(false, true);
		Toast::showTip(tr("uknown error"), this);
	}

	const auto cover = PixmapCache::Instance().find(item.cover_id);
	if (cover != nullptr && !cover->isNull()) {
		setCover(*cover);
		playlist_page_->cover()->setPixmap(Pixmap::resizeImage(*cover, playlist_page_->cover()->size(), true));
	}
	else {
		setCover(QPixmap(Q_UTF8(":/xamp/Resource/White/unknown_album.png")));
	}

	QFileInfo file_info(item.file_path);
	const auto lrc_path = file_info.path() + Q_UTF8("/") + file_info.completeBaseName() + Q_UTF8(".lrc");
	lrc_page_->lyricsWidget()->loadLrcFile(lrc_path);

	playlist_page_->title()->setText(item.title);
	lrc_page_->title()->setText(item.title);
	lrc_page_->album()->setText(item.album);
	lrc_page_->artist()->setText(item.artist);

	if (!player_->IsPlaying()) {
		playlist_page_->format()->setText(Q_UTF8(""));
	}
}

void Xamp::setCover(const QPixmap& cover) {
	assert(!cover.isNull());
	ui.coverLabel->setPixmap(Pixmap::resizeImage(cover, ui.coverLabel->size(), true));
	playlist_page_->cover()->setPixmap(Pixmap::resizeImage(cover, playlist_page_->cover()->size(), true));
	lrc_page_->cover()->setPixmap(Pixmap::resizeImage(cover, lrc_page_->cover()->size(), true));
}

void Xamp::onPlayerStateChanged(xamp::player::PlayerState play_state) {
	if (!player_) {
		return;
	}
	if (play_state == PlayerState::PLAYER_STATE_STOPPED) {
		resetTaskbarProgress();
		ui.seekSlider->setValue(0);
		ui.startPosLabel->setText(Time::msToString(0));
		playNextItem(1);
	}
}

void Xamp::initialPlaylist() {
	int32_t playlist_id = 1;
	IF_FAILED_SHOW_TOAST(Database::Instance().addPlaylist(Q_UTF8(""), 1));
	
	playlist_page_ = newPlaylist(playlist_id);
	playlist_page_->playlist()->setPlaylistId(playlist_id);
	Database::Instance().forEachPlaylistMusic(playlist_id, [this, playlist_id](const auto& entityy) {
		playlist_page_->playlist()->appendItem(entityy);
		});	
	lrc_page_ = new LrcPage(this);

	pushWidget(lrc_page_);
	pushWidget(playlist_page_);
	pushWidget(new AlbumView(this));
	goBackPage();
}

void Xamp::addItem(const QString& file_name) {
	playlist_page_->playlist()->append(file_name);
}

void Xamp::pushWidget(QWidget* widget) {
	auto id = ui.currentView->addWidget(widget);
	stack_page_id_.push(id);
	ui.currentView->setCurrentIndex(id);
}

QWidget* Xamp::topWidget() {
	if (!stack_page_id_.isEmpty()) {
		return ui.currentView->widget(stack_page_id_.top());
	}
	return nullptr;
}

QWidget* Xamp::popWidget() {
	if (!stack_page_id_.isEmpty()) {
		auto id = stack_page_id_.pop();
		auto widget = ui.currentView->widget(id);
		ui.currentView->removeWidget(widget);
		if (!stack_page_id_.isEmpty()) {
			ui.currentView->setCurrentIndex(stack_page_id_.top());
			return widget;
		}
	}
	return nullptr;
}

PlyalistPage* Xamp::newPlaylist(int32_t playlist_id) {
	auto playlist_page = new PlyalistPage(nullptr);
	ui.currentView->addWidget(playlist_page);
	(void)QObject::connect(playlist_page->playlist(), &PlayListTableView::playMusic,
		[this](auto index, const auto& item) {
			play(index, item);
		});
	(void)QObject::connect(playlist_page->playlist(), &PlayListTableView::removeItems,
		[this](auto playlist_id, const auto& select_music_ids) {
			IF_FAILED_SHOW_TOAST(Database::Instance().removePlaylistMusic(playlist_id, select_music_ids));
		});
	auto table_id = Database::Instance().addTable(Q_UTF8(""), 0);
	if (playlist_id == -1) {
		playlist_id = Database::Instance().addPlaylist(Q_UTF8(""), 0);
	}	
	playlist_page->playlist()->setPlaylistId(playlist_id);
	return playlist_page;
}

void Xamp::addDropFileItem(const QUrl& url) {
	addItem(url.toLocalFile());
}