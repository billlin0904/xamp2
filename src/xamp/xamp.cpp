#include <qDebug>
#include <QToolTip>
#include <QFontDatabase>
#include <QMenu>

#include "widget/win32/blur_effect_helper.h"
#include "widget/image_utiltis.h"
#include "widget/time_utilts.h"
#include "widget/pixmapcache.h"

#include "xamp.h"

Xamp::Xamp(QWidget *parent)
	: FramelessWindow(parent)
	, is_seeking_(false) {
	player_ = std::make_shared<AudioPlayer>();
	state_adapter_ = std::make_shared<PlayerStateAdapter>();
	player_->SetStateAdapter(state_adapter_);
	initialUI();
	initialController();
	initialDeviceList();
}

void Xamp::initialUI() {
	ui.setupUi(this);

	ui.currentView->setStyleSheet("background-color: rgba(228, 233, 237, 230);");	
	ui.titleFrame->setStyleSheet("background-color: rgba(228, 233, 237, 230);");

	ui.sliderBar->setStyleSheet("background-color: rgba(228, 233, 237, 150);");

	ui.controlFrame->setStyleSheet("background-color: rgba(228, 233, 237, 240);");
	ui.volumeFrame->setStyleSheet("background-color: rgba(228, 233, 237, 240);");
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
			background_color: white;
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
			background: transparent;
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
			QObject::connect(device_action, &QAction::triggered, [device_info, this]() {
				device_info_ = device_info;
				});
			menu->addAction(device_action);
		}

		if (!is_find_setting_device) {
			auto itr = std::find_if(device_info_list.begin(), device_info_list.end(), [](const auto& info) {
				return info.is_default_device;
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

	QObject::connect(ui.minWinButton, &QToolButton::pressed, [this]() {
		showMinimized();
		});

	QObject::connect(ui.maxWinButton, &QToolButton::pressed, [this]() {
		if (isMaximized()) {
			showNormal();
		}
		else {
			showMaximized();
		}
		});

	QObject::connect(ui.closeButton, &QToolButton::pressed, [this]() {
		QWidget::close();
		});

	QObject::connect(ui.seekSlider, &QSlider::sliderMoved, [this](auto value) {
		QToolTip::showText(QCursor::pos(), Time::msToString(double(ui.seekSlider->value()) / 1000.0));
		if (!is_seeking_) {
			return;
		}
		ui.seekSlider->setValue(value);
		});

	QObject::connect(ui.seekSlider, &QSlider::sliderReleased, [this]() {
		QToolTip::showText(QCursor::pos(), Time::msToString(double(ui.seekSlider->value()) / 1000.0));
		if (!is_seeking_) {
			return;
		}
		player_->Seek(static_cast<double>(ui.seekSlider->value() / 1000.0));
		is_seeking_ = false;
		});

	QObject::connect(ui.seekSlider, &QSlider::sliderPressed, [this]() {
		QToolTip::showText(QCursor::pos(), Time::msToString(double(ui.seekSlider->value()) / 1000.0));
		if (is_seeking_) {
			return;
		}
		is_seeking_ = true;
		player_->Pause();
		});

	ui.volumeSlider->setRange(0, 100);
	ui.volumeSlider->setValue(50);

	QObject::connect(ui.volumeSlider, &QSlider::sliderMoved, [this](auto pos) {
		});

	QObject::connect(ui.volumeSlider, &QSlider::valueChanged, [this](auto volume) {
		QToolTip::showText(QCursor::pos(), tr("Volume : ") + QString::number(volume) + QString("%"));
		setVolume(volume);
		});

	QObject::connect(ui.volumeSlider, &QSlider::sliderMoved, [this](auto volume) {
		QToolTip::showText(QCursor::pos(), tr("Volume : ") + QString::number(volume) + QString("%"));
		setVolume(volume);
		});

	QObject::connect(state_adapter_.get(),
		&PlayerStateAdapter::stateChanged,
		this,
		&Xamp::onPlayerStateChanged,
		Qt::QueuedConnection);

	QObject::connect(state_adapter_.get(),
		&PlayerStateAdapter::sampleTimeChanged,
		this,
		&Xamp::onSampleTimeChanged,
		Qt::QueuedConnection);

	QObject::connect(ui.searchLineEdit, &QLineEdit::textChanged, [this](const auto &text) {		
		if (ui.currentView->count() > 0) {
			auto playlist_view = static_cast<PlayListTableView*>(ui.currentView->widget(0));
			emit playlist_view->search(text, Qt::CaseSensitivity::CaseInsensitive, QRegExp::PatternSyntax());
		}
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
	player_->SetVolume(volume);
}

void Xamp::addMusic(int32_t music_id, PlayListTableView* playlist) {
}

void Xamp::playNextClicked() {
}

void Xamp::playPreviousClicked() {
}

void Xamp::onSampleTimeChanged(double stream_time) {
	if (player_->GetState() == PlayerState::PLAYER_STATE_RUNNING) {
		ui.endPosLabel->setText(Time::msToString(player_->GetDuration() - stream_time));
		auto stream_time_as_ms = static_cast<int32_t>(stream_time * 1000.0);
		ui.seekSlider->setValue(stream_time_as_ms);
		ui.startPosLabel->setText(Time::msToString(stream_time));
		setTaskbarProgress(100.0 * ui.seekSlider->value() / ui.seekSlider->maximum());
	}
}

void Xamp::playLocalFile(const PlayListEntity& item) {
	setTaskbarPlayerPlaying();
	play(item);
}

void Xamp::play(const PlayListEntity& item) {
	ui.titleLabel->setText(item.title);
	ui.artistLabel->setText(item.artist);

	player_->Open(item.file_path.toStdWString(), device_info_);
	player_->SetVolume(ui.volumeSlider->value());
	
	ui.seekSlider->setValue(0);
	ui.seekSlider->setEnabled(true);
	ui.seekSlider->setRange(0, int32_t(player_->GetDuration() * 1000));
	ui.startPosLabel->setText(Time::msToString(0));
	ui.endPosLabel->setText(Time::msToString(player_->GetDuration()));

	player_->PlayStream();
}

void Xamp::play(const QModelIndex& index, const PlayListEntity& item) {
	try {
		playLocalFile(item);
		setPlayOrPauseButton(true);
	} catch(const xamp::base::Exception& e) {
		ui.seekSlider->setEnabled(false);
		player_->Stop(true, true);
		qDebug() << e.GetErrorMessage();
	} catch (const std::exception& e) {
		ui.seekSlider->setEnabled(false);
		player_->Stop(true, true);
		qDebug() << e.what();
	} catch (...) {
		ui.seekSlider->setEnabled(false);
		player_->Stop(true, true);
	}

	QPixmap cover;
	if (PixmapCache::Instance().find(item.cover_id, cover)) {
		assert(!cover.isNull());
		auto cover_size = ui.coverLabel->size();
		ui.coverLabel->setPixmap(Pixmap::resizeImage(cover, cover_size, true));
	}
}

void Xamp::onPlayerStateChanged(xamp::player::PlayerState play_state) {
}

void Xamp::addItem(const QString& file_name) {
	PlayListTableView* playlist_view = nullptr;
	if (!ui.currentView->count()) {
		playlist_view = new PlayListTableView(nullptr);
		playlist_view->setStyleSheet(R"(background: transparent;)");
		QObject::connect(playlist_view, &PlayListTableView::playMusic, [this](auto index, const auto &item) {
			play(index, item);
			});
		ui.currentView->addWidget(playlist_view);
	}
	else {
		playlist_view = static_cast<PlayListTableView*>(ui.currentView->widget(0));
	}
	playlist_view->append(file_name);
}

void Xamp::addDropFileItem(const QUrl& url) {
	addItem(url.toLocalFile());
}