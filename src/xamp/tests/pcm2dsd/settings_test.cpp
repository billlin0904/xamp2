#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QTranslator>
#include <widget/preferencepage.h>
#include <widget/pcmdsdsettings.h>
#include <widget/jsonsettings.h>
#include <base/logger.h>
#include <iostream>

void require(bool value,const char* text) { if(!value) throw std::runtime_error(text); }
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    try {
        XampLoggerFactory.startup();
        QTemporaryDir dir;
        require(dir.isValid(),"temporary settings directory");
        qAppSettings.loadIniFile(dir.filePath(QStringLiteral("test.ini")));
        qJsonSettings.loadJsonFile(dir.filePath(QStringLiteral("test.json")));
        QTranslator translator;
        require(translator.load(QCoreApplication::applicationDirPath()+QStringLiteral("/langs/widget_shared_zh_TW.qm")),"translation file");
        app.installTranslator(&translator);
        {
            PreferencePage page;
            auto* bit=page.findChild<QCheckBox*>(QStringLiteral("bitPerfectCheckBox"));
            auto* enable=page.findChild<QCheckBox*>(QStringLiteral("pcm2DsdEnableCheckBox"));
            auto* rate=page.findChild<QComboBox*>(QStringLiteral("pcm2DsdRateComboBox"));
            auto* gain=page.findChild<QDoubleSpinBox*>(QStringLiteral("pcm2DsdGainSpinBox"));
            auto* dither=page.findChild<QCheckBox*>(QStringLiteral("pcm2DsdDitherCheckBox"));
            auto* transport=page.findChild<QComboBox*>(QStringLiteral("pcm2DsdTransportComboBox"));
            require(transport && transport->currentData().toInt()==0&&!transport->isEnabled(),"default DoP transport/gating");
            auto* filter=page.findChild<QComboBox*>(QStringLiteral("pcm2DsdFilterComboBox"));
            auto* modulator=page.findChild<QComboBox*>(QStringLiteral("pcm2DsdModulatorComboBox"));
            auto* block=page.findChild<QComboBox*>(QStringLiteral("pcm2DsdBlockComboBox"));
            require(bit&&enable&&rate&&gain&&dither&&filter&&modulator&&block,"settings controls");
            require(filter->currentData().toInt()==0&&modulator->currentData().toInt()==0&&block->currentData().toInt()==256,"DSP defaults");
            require(!filter->isEnabled()&&!modulator->isEnabled()&&!block->isEnabled(),"DSP gate off");
            require(!enable->isEnabled()&&!rate->isEnabled(),"BitPerfect gate initially off");
            require(rate->currentData().toInt()==64&&gain->value()==0&&dither->isChecked(),"defaults");
            bit->setChecked(true);require(enable->isEnabled()&&!rate->isEnabled(),"converter off gate");
            enable->setChecked(true);require(rate->isEnabled(),"converter on gate");
            rate->setCurrentIndex(rate->findData(128));gain->setValue(-3.5);dither->setChecked(false);
            transport->setCurrentIndex(transport->findData(1));
            require(rate->currentText().contains(QStringLiteral("MHz")),"native rate labels");
            filter->setCurrentIndex(filter->findData(2));
            modulator->setCurrentIndex(modulator->findData(1));
            block->setCurrentIndex(block->findData(1024));
            auto settings=pcm_dsd_settings::load();pcm_dsd_settings::validate(settings);
            require(settings.enabled&&settings.multiplier==128&&settings.gain_db==-3.5&&!settings.dither,"stored parameters");
            require(settings.filter==2&&settings.modulator==1&&settings.block_frames==1024,"stored DSP parameters");
            bit->setChecked(false);require(!filter->isEnabled()&&!modulator->isEnabled()&&!block->isEnabled(),"DSP gate reset");require(!enable->isEnabled()&&!gain->isEnabled(),"disabled when BitPerfect off");
            bit->setChecked(true);
            page.findChild<QStackedWidget*>(QStringLiteral("stackedWidget"))->setCurrentWidget(page.findChild<QWidget*>(QStringLiteral("outputSettingsPage")));
            page.resize(920,680);page.show();app.processEvents();
            require(page.grab().save(QCoreApplication::applicationDirPath()+QStringLiteral("/pcm2dsd-settings.png")),"settings screenshot");
            qAppSettings.save();
        }
        qAppSettings.loadIniFile(dir.filePath(QStringLiteral("test.ini")));
        const auto restored=pcm_dsd_settings::load();
        require(restored.enabled&&restored.multiplier==128&&restored.gain_db==-3.5&&!restored.dither,"settings survive reload");
        require(restored.transport==1,"transport survives reload");
        require(restored.filter==2&&restored.modulator==1&&restored.block_frames==1024,"DSP settings survive reload");
        auto invalid=restored;invalid.block_frames=255;
        bool rejected=false;try { pcm_dsd_settings::validate(invalid); } catch(const std::runtime_error&) { rejected=true; }
        require(rejected,"reject invalid block setting");
        std::cout<<"PASS: settings controls, BitPerfect gating, defaults, parameter persistence, translated render\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
