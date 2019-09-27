#include <qDebug>
#include <QToolTip>
#include <QFontDatabase>
#include <QMenu>
#include <QCloseEvent>
#include <QMessageBox>
#include <QFileInfo>

#include "widget/win32/blur_effect_helper.h"

#include "widget/playlistpage.h"
#include "widget/toast.h"
#include "widget/image_utiltis.h"
#include "widget/time_utilts.h"
#include "widget/database.h"
#include "widget/pixmapcache.h"

#include "xamp.h"

Xamp::Xamp(QWidget *parent)
	: FramelessWindow(parent)
	, is_seeking_(false)
	, order_(PLAYER_ORDER_REPEAT)
	, lrc_page_(nullptr)
	, state_adapter_(std::make_shared<PlayerStateAdapter>())
	, player_(std::make_shared<AudioPlayer>(state_adapter_)) {
	PixmapCache::Instance();
	initialUI();
	initialController();
	initialDeviceList();
}

void Xamp::closeEvent(QCloseEvent* event) {
	player_->Stop(false, true);
	player_.reset();
}

void Xamp::initialUI() {
	ui.setupUi(this);

	ui.currentView->setStyleSheet("background-color: rgba(228, 233, 237, 230);");	
	ui.titleFrame->setStyleSheet("background-color: rgba(228, 233, 237, 230);");

	ui.sliderBar->setStyleSheet("background-color: rgba(228, 233, 237, 150);");

	ui.controlFrame->setStyleSheet("background-color: rgba(255, 255, 255, 200);");
	ui.volumeFrame->setStyleSheet("background-color: rgba(255, 255, 255, 200);");

	ui.playingFrame->setStyleSheet("background-color: rgba(228, 233, 237, 220);");
	ui.searchLineEdit->setStyleSheet("background: transparent;");
	
	ui.closeButton->setStyleSheet(R"(
		QToolButton#closeButton { 
			image: url(:/xamp/Resource/White/close.png);
			background: transparent;
			background-color: rgba(255, 255, 255, 0);
		}
	)");
	ui.minWinButton->setStyleSheet(R"(
		QToolButton#minWinButton { 
			image: url(:/xamp/Resource/White/minimize.png);
			background: transparent; 
			background-color: rgba(255, 255, 255, 0);
		}
	)");
	ui.maxWinButton->setStyleSheet(R"(
		QToolButton#maxWinButton { 
			image: url(:/xamp/Resource/White/maximize.png);
			background: transparent;
			background-color: rgba(255, 255, 255, 0);
		}
	)");

	setPlayOrPauseButton(false);
	
	ui.nextButton->setStyleSheet(R"(
		QToolButton#nextButton { 
			image: url(:/xamp/Resource/White/next.png);
			background: transparent;
		 }
	)");
	ui.prevButton->setStyleSheet(R"(
		QToolButton#prevButton {
			image: url(:/xamp/Resource/White/previous.png);
			background: transparent;
		 }
	)");

	ui.nextPageButton->setStyleSheet(R"(
		QToolButton#nextPageButton {
			image: url(:/xamp/Resource/White/right_black.png);
			background: transparent;
		}
	)");
	ui.backPageButton->setStyleSheet(R"(
		QToolButton#backPageButton {
			image: url(:/xamp/Resource/White/left_black.png);
			background: transparent;
		}
	)");
	ui.searchLineEdit->setStyleSheet(R"(
		QLineEdit#searchLineEdit {
			background-color: white;
			border: none;
		}
	)");
	ui.mutedButton->setStyleSheet(R"(
		QToolButton#mutedButton {
			image: url(:/xamp/Resource/White/volume_up.png);
			background: transparent;
		}
	)");
	ui.selectDeviceButton->setStyleSheet(R"(
		QToolButton#selectDeviceButton {
			image: url(:/xamp/Resource/White/speaker.png);
			background: transparent;
		}
	)");
	ui.playlistButton->setStyleSheet(R"(
		QToolButton#playlistButton {
			background: transparent;
		}
	)");
	ui.volumeSlider->setStyleSheet(R"(
		QSlider#volumeSlider {
			background: transparent;
		}
	)");

	ui.addPlaylistButton->setStyleSheet(R"(
		QToolButton#addPlaylistButton {
			background: transparent;
		}
	)");
	ui.repeatButton->setStyleSheet(R"(
		QToolButton#repeatButton {
			image: url(:/xamp/Resource/White/repeat.png);
			background: transparent;
		}
	)");
	ui.addPlaylistButton->setStyleSheet(R"(
		QToolButton#addPlaylistButton {
			image: url(:/xamp/Resource/White/create_new_folder.png);
			background: transparent;
		}
	)");

	ui.titleLabel->setStyleSheet(R"(
		QLabel#titleLabel {
			background: transparent;
		}
	)");
	ui.artistLabel->setStyleSheet(R"(
		QLabel#artistLabel {
			background: transparent;
		}
	)");
	ui.startPosLabel->setStyleSheet(R"(
		QLabel#startPosLabel {
			background: transparent;
			color: gray;
		}
	)");
	ui.endPosLabel->setStyleSheet(R"(
		QLabel#endPosLabel {
			background: transparent;
			color: gray;
		}
	)");
	ui.seekSlider->setStyleSheet(R"(
		QSlider#seekSlider {
			background: transparent;
		}
	)");
	ui.artistLabel->setStyleSheet(R"(
		QLabel#artistLabel {
			background: transparent;
			color: gray;
		}
	)");
}

void Xamp::setPlayOrPauseButton(bool is_playing) {
	if (is_playing) {
		ui.playButton->setStyleSheet(R"(
		QToolButton#playButton {
			image: url(:/xamp/Resource/White/pause.png);
			background: transparent;
		}
		)");
	}
	else {
		ui.playButton->setStyleSheet(R"(
		QToolButton#playButton {
			image: url(:/xamp/Resource/White/play.png);
			background: transparent;
		}
		)");
	}
}

QWidgetAction* Xamp::createTextSeparator(const QString& text) {
	auto label = new QLabel(text);
	label->setObjectName("textSeparator");
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

	menu->setStyleSheet(R"(		
		QMenu {
			background-color: rgba(228, 233, 237, 150);
		}
		QMenu::item:selected { 
			background-color: black;
		}
		)");
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
				});
			menu->addAction(device_action);
		}

		if (!is_find_setting_device) {
			auto itr = std::find_if(device_info_list.begin(), device_info_list.end(), [](const auto& info) {
				return info.is_default_device && !DeviceFactory::IsExclusiveDevice(info);
				});
			if (itr != device_info_list.end()) {			
				init_device_info = (*itr);
				device_id_action[init_device_info.device_id]->setChecked(true);
			}
		}
		});

	device_info_ = init_device_info;
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

	(void)QObject::connect(ui.seekSlider, &QSlider::sliderMoved, [this](auto value) {
		QToolTip::showText(QCursor::pos(), Time::msToString(double(ui.seekSlider->value()) / 1000.0));
		if (!is_seeking_) {
			return;
		}
		ui.seekSlider->setValue(value);
		});

	(void)QObject::connect(ui.seekSlider, &QSlider::sliderReleased, [this]() {
		QToolTip::showText(QCursor::pos(), Time::msToString(double(ui.seekSlider->value()) / 1000.0));
		if (!is_seeking_) {
			return;
		}
		try {
			player_->Seek(static_cast<double>(ui.seekSlider->value() / 1000.0));
		}
		catch (const xamp::base::Exception& e) {
			player_->Stop(false);
			Toast::showTip(e.GetErrorMessage(), this);
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

	ui.volumeSlider->setRange(0, 100);
	ui.volumeSlider->setValue(50);

	(void)QObject::connect(ui.volumeSlider, &QSlider::sliderMoved, [this](auto pos) {
		});

	(void)QObject::connect(ui.volumeSlider, &QSlider::valueChanged, [this](auto volume) {
		QToolTip::showText(QCursor::pos(), tr("Volume : ") + QString::number(volume) + QString("%"));
		setVolume(volume);
		});

	(void)QObject::connect(ui.volumeSlider, &QSlider::sliderMoved, [this](auto volume) {
		QToolTip::showText(QCursor::pos(), tr("Volume : ") + QString::number(volume) + QString("%"));
		setVolume(volume);
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
			auto playlist_view = static_cast<PlyalistPage*>(ui.currentView->widget(0))->playlist();
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
		setPlayerOrder();
		});

	(void)QObject::connect(ui.playButton, &QToolButton::pressed, [this]() {
		play();
		});

	(void)QObject::connect(ui.backPageButton, &QToolButton::pressed, [this]() {
		auto idx = ui.currentView->currentIndex();
		if (idx > 0) {
			ui.currentView->setCurrentIndex(idx - 1);
		}
		});


	(void)QObject::connect(ui.coverLabel, &ClickableLabel::clicked, [this]() {
		ui.currentView->setCurrentIndex(1);
		});

	ui.seekSlider->setEnabled(false);
	ui.startPosLabel->setText(Time::msToString(0));
	ui.endPosLabel->setText(Time::msToString(0));
}

void Xamp::setVolume(int32_t volume) {
	if (volume > 0) {
		player_->SetMute(false);
		ui.mutedButton->setIcon(QIcon(":/xamp/Resource/White/volume_up.png"));
	}
	else {
		player_->SetMute(true);
		ui.mutedButton->setIcon(QIcon(":/xamp/Resource/White/volume_off.png"));
	}
	try {
		player_->SetVolume(volume);
	}
	catch (const xamp::base::Exception& e) {
		player_->Stop(false);
		Toast::showTip(e.GetErrorMessage(), this);
		return;
	}
}

void Xamp::playNextClicked() {
	playNextItem(1);
}

void Xamp::playPreviousClicked() {
	playNextItem(-1);
}

void Xamp::setPlayerOrder() {
	order_ = static_cast<PlayerOrder>((order_ + 1) % _MAX_PLAYER_ORDER_);
	switch (order_) {
	case PLAYER_ORDER_REPEAT:
		ui.repeatButton->setStyleSheet(R"(
		QToolButton#repeatButton {
			image: url(:/xamp/Resource/White/repeat.png);
			background: transparent;
		}
		)");
		break;
	case PLAYER_ORDER_REPEAT_ONE:
		ui.repeatButton->setStyleSheet(R"(
		QToolButton#repeatButton {
			image: url(:/xamp/Resource/White/repeat_one.png);
			background: transparent;
		}
		)");
		break;
	case PLAYER_ORDER_SHUFFLE_ALL:
		ui.repeatButton->setStyleSheet(R"(
		QToolButton#repeatButton {
			image: url(:/xamp/Resource/White/shuffle.png);
			background: transparent;
		}
		)");
		break;
	}
}

void Xamp::playNextItem(int32_t forward) {
	if (!ui.currentView->count()) {
		stopPlayedClicked();
		return;
	}

	auto playlist_view = static_cast<PlyalistPage*>(ui.currentView->widget(0))->playlist();
	const auto count = playlist_view->model()->rowCount();
	if (count == 0) {
		stopPlayedClicked();
		return;
	}

	switch (order_) {
	case PLAYER_ORDER_REPEAT_ONE:
		play_index_ = playlist_view->currentIndex();
		break;
	case PLAYER_ORDER_REPEAT:
		play_index_ = playlist_view->model()->index(play_index_.row() + forward, PLAYLIST_PLAYING);
		if (play_index_.row() == -1) {
			return;
		}
		break;
	case PLAYER_ORDER_SHUFFLE_ALL:
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
	if (player_->GetState() == PlayerState::PLAYER_STATE_RUNNING) {
		ui.endPosLabel->setText(Time::msToString(player_->GetDuration() - stream_time));
		auto stream_time_as_ms = static_cast<int32_t>(stream_time * 1000.0);
		ui.seekSlider->setValue(stream_time_as_ms);
		ui.startPosLabel->setText(Time::msToString(stream_time));
		setTaskbarProgress(100.0 * ui.seekSlider->value() / ui.seekSlider->maximum());
		lrc_page_->setLrcTime(stream_time_as_ms);
	}
}

void Xamp::playLocalFile(const PlayListEntity& item) {
	setTaskbarPlayerPlaying();
	play(item);
}

void Xamp::play() {
	if (player_->GetState() == PLAYER_STATE_RUNNING) {
		setPlayOrPauseButton(false);
		player_->Pause();
		setTaskbarPlayerPaused();
	}
	else if (player_->GetState() == PLAYER_STATE_PAUSED) {
		setPlayOrPauseButton(true);
		player_->Resume();
		setTaskbarPlayingResume();
	}
	else if (player_->GetState() == PLAYER_STATE_STOPPED) {
		if (!ui.currentView->count()) {
			return;
		}
		auto playlist_page = static_cast<PlyalistPage*>(ui.currentView->widget(0));
		if (auto select_item = playlist_page->playlist()->selectItem()) {
			play_index_ = select_item.value();
		}
		play_index_ = playlist_page->playlist()->model()->index(
			play_index_.row(), PLAYLIST_PLAYING);
		if (play_index_.row() == -1) {
			Toast::showTip(tr("Not found any playing item."), this);
			return;
		}
		playlist_page->playlist()->setNowPlaying(play_index_, true);
		playlist_page->playlist()->play(play_index_);
	}
}

void Xamp::play(const PlayListEntity& item) {
	ui.titleLabel->setText(item.title);
	ui.artistLabel->setText(item.artist);

	const QString bass_support_ext(".dsd,.dsf,.dff,.m4a");
	auto use_bass_stream = bass_support_ext.contains(item.file_ext);
	player_->Open(item.file_path.toStdWString(), use_bass_stream, device_info_);
	
	if (!player_->IsMute()) {		
		setVolume(ui.volumeSlider->value());
	} else {
		setVolume(0);
	}
	
	ui.seekSlider->setValue(0);
	ui.seekSlider->setEnabled(true);
	ui.seekSlider->setRange(0, int32_t(player_->GetDuration() * 1000));
	ui.startPosLabel->setText(Time::msToString(0));
	ui.endPosLabel->setText(Time::msToString(player_->GetDuration()));

	QFileInfo file_info(item.file_path);
	const auto lrc_path = file_info.path() + "/" + file_info.completeBaseName() + ".lrc";
	lrc_page_->loadLrcFile(lrc_path);

	player_->PlayStream();	
}

void Xamp::play(const QModelIndex& index, const PlayListEntity& item) {
	try {		
		playLocalFile(item);
		setPlayOrPauseButton(true);
	} catch(const xamp::base::Exception& e) {
		ui.seekSlider->setEnabled(false);
		player_->Stop(false);
		Toast::showTip(e.GetErrorMessage(), this);
		return;
	} catch (const std::exception& e) {
		ui.seekSlider->setEnabled(false);
		player_->Stop(false);
		Toast::showTip(e.what(), this);
		return;
	} catch (...) {
		ui.seekSlider->setEnabled(false);
		player_->Stop(false);
		Toast::showTip(tr("uknown error"), this);
		return;
	}
	
	auto playlist_page = static_cast<PlyalistPage*>(ui.currentView->widget(0));
	QPixmap cover;
	if (!PixmapCache::Instance().find(item.cover_id, cover)) {
		cover = QPixmap(":/xamp/Resource/White/unknown_album.png");
	}
	setCover(cover);
	playlist_page->title()->setText(item.title);
}

void Xamp::setCover(const QPixmap& cover) {
	assert(!cover.isNull());
	auto playlist_page = static_cast<PlyalistPage*>(ui.currentView->widget(0));
	ui.coverLabel->setPixmap(Pixmap::resizeImage(cover, ui.coverLabel->size(), true));
	playlist_page->cover()->setPixmap(Pixmap::resizeImage(cover, playlist_page->cover()->size(), true));
}

void Xamp::onPlayerStateChanged(xamp::player::PlayerState play_state) {
	if (play_state == PlayerState::PLAYER_STATE_STOPPED) {
		resetTaskbarProgress();
		ui.seekSlider->setValue(0);
		ui.startPosLabel->setText(Time::msToString(0));
		playNextItem(1);
	}
}

void Xamp::addItem(const QString& file_name) {
	PlyalistPage* playlist_page = nullptr;
	if (!ui.currentView->count()) {
		playlist_page = new PlyalistPage(nullptr);
		ui.currentView->addWidget(playlist_page);
		(void)QObject::connect(playlist_page->playlist(), &PlayListTableView::playMusic,
			[this](auto index, const auto& item) {
				play(index, item);
			});
		auto table_id = Database::Instance().addTable("", 0);
		auto playlist_id = Database::Instance().addPlaylist("", 0);
		playlist_page->playlist()->setPlaylistId(playlist_id);
		lrc_page_ = new LyricsShowWideget(this);		
		ui.currentView->addWidget(lrc_page_);
		setCover(QPixmap(":/xamp/Resource/White/unknown_album.png"));
	}
	else {
		playlist_page = static_cast<PlyalistPage*>(ui.currentView->widget(0));
	}
	playlist_page->playlist()->append(file_name);
}

void Xamp::addDropFileItem(const QUrl& url) {
	addItem(url.toLocalFile());
}