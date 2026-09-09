#include <stream/api.h>
#include <stream/idspmanager.h>
#include <base/logger.h>
#include <base/dll.h>
#include <base/fs.h>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <limits>
using namespace xamp::base;
using namespace xamp::stream;
namespace {
void initialize() {
    static const bool ready = [] {
        XampLoggerFactory.startup();
        REQUIRE(addSharedLibrarySearchDirectory(getComponentsFilePath()));
        return true;
    }();
    (void)ready;
}
}


#include <stream/bassparametriceq.h>
TEST_CASE("BASS EQ gain and invalid update rollback", "[eq]") {
    initialize();
    static const bool loaded=[] { loadBassLib(); return true; }();
    (void)loaded;
    BassParametricEq eq;
    Property config;
    config.create(DspConfig::kOutputFormat, AudioFormat(DataFormat::FORMAT_PCM, 2, ByteFormat::FLOAT32, 48000));
    EqSettings settings;
    settings.bands.push_back({EQFilterTypes::FT_ALL_PEAKING_EQ,1000,6,0,1.41f,0});
    config.create(DspConfig::kEQSettings,settings);
    REQUIRE_NOTHROW(eq.initialize(config));
    const auto gain = [&] {
        std::vector<float> input(48000*2);
        for(size_t i=0;i<input.size()/2;++i) input[i*2]=input[i*2+1]=0.1f*std::sin(2*3.141592653589793*1000*i/48000);
        Buffer<float> storage(input.size()); BufferRef<float> output(storage);
        REQUIRE(eq.process(input.data(),input.size(),output));
        double a=0,b=0;
        for(size_t i=24000;i<input.size();++i) {a+=input[i]*input[i];b+=output.data()[i]*output.data()[i];}
        return 10*std::log10(b/a);
    };
    REQUIRE(std::abs(gain()-6)<0.05);
    auto invalid=settings; invalid.bands.push_back({EQFilterTypes::FT_LOW_PASS,24000,0,0,1,0});
    REQUIRE_THROWS(eq.setEq(invalid));
    REQUIRE(std::abs(gain()-6)<0.05);
    settings.bands.clear(); settings.preamp=-6;
    REQUIRE_NOTHROW(eq.setEq(settings));
    REQUIRE(std::abs(gain()+6)<0.05);
    settings.bands.push_back({EQFilterTypes::FT_LOW_SHELF,1000,6,0,1.41f,0});
    REQUIRE_NOTHROW(eq.setEq(settings));
    REQUIRE(std::abs(gain()+3)<0.05);
    settings.bands[0].shelf_slope=1;
    REQUIRE_NOTHROW(eq.setEq(settings));
    REQUIRE(std::abs(gain()+3)<0.05);
    settings.preamp=std::numeric_limits<float>::quiet_NaN();
    REQUIRE_THROWS(eq.setEq(settings));
    REQUIRE(std::abs(gain()+3)<0.05);
}
namespace {
class ArithmeticDsp final : public IAudioProcessor {
public:
    ArithmeticDsp(uint8_t id,float add,bool fail=false):id_(id),add_(add),fail_(fail) {}
    Uuid getTypeId() const override { UuidBuffer b{};b[0]=id_;return Uuid(b); }
    std::string_view getDescription() const override {return "Test arithmetic";}
    void initialize(const Property&) override {}
    bool process(const float* input,size_t n,BufferRef<float>& out) override {
        if(fail_) return false;
        out.maybeResize(n);
        for(size_t i=0;i<n;++i) out.data()[i]=input[i]*2+add_;
        return true;
    }
private: uint8_t id_;float add_;bool fail_;
};
}
TEST_CASE("DSP stages compose in order and failed post DSP writes no audio", "[dsp]") {
    initialize();
    for(bool fail:{false,true}) {
        auto dsp=StreamFactory::makeDSPManager();
        dsp->addPreDSP(makeAlign<IAudioProcessor,ArithmeticDsp>(1,1.f));
        dsp->addPreDSP(makeAlign<IAudioProcessor,ArithmeticDsp>(2,2.f));
        dsp->addPostDSP(makeAlign<IAudioProcessor,ArithmeticDsp>(3,3.f));
        dsp->addPostDSP(makeAlign<IAudioProcessor,ArithmeticDsp>(4,4.f,fail));
        Property config;config.create(DspConfig::kSampleSize,uint32_t(4));config.create(DspConfig::kDsdMode,DsdModes::DSD_MODE_PCM);
        dsp->initialize(config);AudioBuffer<std::byte> fifo(1024);float input[]={1,2};
        REQUIRE(dsp->processDSP(input,2,fifo)==fail);
        if(fail) REQUIRE(fifo.getAvailableRead()==0);
        else {
            float output[2]{};size_t count=0;
            REQUIRE(fifo.tryRead(reinterpret_cast<std::byte*>(output),sizeof(output),count));
            REQUIRE(output[0]==42);REQUIRE(output[1]==58);
        }
    }
}
