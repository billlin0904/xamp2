#include <stream/api.h>
#include <stream/avlibfilestream.h>
#include <base/pcm.h>
#include <stream/idspmanager.h>
#include <base/bitperfect.h>
#include <base/logger.h>
#include <base/dll.h>
#include <base/fs.h>
#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <vector>
#include <array>
#include <cmath>
using namespace xamp::base;
using namespace xamp::stream;
namespace {
void initialize() {
    static const bool ready = [] {
        XampLoggerFactory.startup();
        REQUIRE(addSharedLibrarySearchDirectory(getComponentsFilePath()));
        loadAvLib();
        return true;
    }();
    (void)ready;
}
std::vector<std::byte> read(const Path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    REQUIRE(f.good());
    std::vector<std::byte> data(static_cast<size_t>(f.tellg()));
    f.seekg(0); f.read(reinterpret_cast<char*>(data.data()),data.size());
    return data;
}
class ForbiddenDsp final : public IAudioProcessor {
public:
    Uuid getTypeId() const override { return {}; }
    std::string_view getDescription() const override { return "Must not run in BitPerfect"; }
    void initialize(const Property&) override { throw std::runtime_error("DSP initialized"); }
    bool process(const float*, size_t, BufferRef<float>&) override { throw std::runtime_error("DSP ran"); }
};
}

TEST_CASE("Production integer WAV and FLAC decoding DSP bypass and FIFO preserve all 32 bits", "[pipeline]") {
    initialize();
    for (unsigned bits : {16u,24u,32u}) for (const auto* extension : {"wav","flac"}) {
        CAPTURE(bits,extension);
        const auto stem=std::string("pcm")+std::to_string(bits);const Path root(XAMP_TEST_FIXTURES);
        const auto expected=read(root/(stem+".raw"));
        AvLibFileStream source;source.setIntegerPcm(true);source.openFile(root/(stem+"."+extension));
        const auto format=source.integerPcmFormat();
        REQUIRE(format.has_value()); REQUIRE(format->valid_bits==bits);
        REQUIRE(format->container_bits==32); REQUIRE(format->sample_rate==44100);
        REQUIRE(source.getFormat().getByteFormat()==ByteFormat::SINT32);
        auto dsp=StreamFactory::makeDSPManager();
        dsp->addPreDSP(makeAlign<IAudioProcessor,ForbiddenDsp>());
        dsp->addPostDSP(makeAlign<IAudioProcessor,ForbiddenDsp>());
        dsp->setBitPerfect(true);
        Property config;config.create(DspConfig::kSampleSize,uint32_t(4));
        config.create(DspConfig::kDsdMode,DsdModes::DSD_MODE_PCM);
        REQUIRE_NOTHROW(dsp->initialize(config)); REQUIRE_FALSE(dsp->canProcess());
        AudioBuffer<std::byte> fifo(65537);
        REQUIRE_THROWS(dsp->processDSP(nullptr,0,fifo)); // Float is not a valid strict input.
        std::vector<std::byte> decoded(2054*4),captured(decoded.size()),actual;
        auto packed=*format;packed.container_bits=static_cast<uint16_t>(bits);
        for (;;) {
            const auto block=source.readPcm(decoded);if (!block.frames) break;
            dsp->processPcm(block,fifo);
            size_t count=0;REQUIRE(fifo.tryRead(captured.data(),block.data.size(),count));
            REQUIRE(count==block.data.size());
            const auto offset=actual.size();actual.resize(offset+block.frames*packed.frameBytes());
            std::span<std::byte> destination{actual.data()+offset,block.frames*packed.frameBytes()};
            REQUIRE(xamp::pcm::convert({{captured.data(),count},block.frames,*format},packed,{&destination,1}));
        }
        REQUIRE((actual==expected)); // Includes the entire non-aligned final block.
        for (double position : {0.5,0.123,0.0}) {
            CAPTURE(position);source.seek(position);
            const auto block=source.readPcm(std::span<std::byte>{decoded.data(),400});
            std::vector<std::byte> output(block.frames*packed.frameBytes());std::span<std::byte> destination{output};
            REQUIRE(xamp::pcm::convert(block,packed,{&destination,1}));
            const auto offset=static_cast<size_t>(std::llround(position*44100))*packed.frameBytes();
            REQUIRE(std::equal(output.begin(),output.end(),expected.begin()+offset));
        }
        dsp->setBitPerfect(false);REQUIRE_THROWS(dsp->initialize(config));
    }
}
TEST_CASE("Strict decoding refuses float PCM and implicit mono remapping", "[pipeline][policy]") {
    initialize();
    for (const auto* name:{"float32.wav","mono.wav"}) {
        CAPTURE(name);AvLibFileStream source;source.setIntegerPcm(true);
        REQUIRE_THROWS(source.openFile(Path(XAMP_TEST_FIXTURES)/name));
    }
}
TEST_CASE("Normal decoding still exposes float samples", "[pipeline][normal]") {
    initialize();AvLibFileStream source;source.openFile(Path(XAMP_TEST_FIXTURES)/"pcm16.wav");
    REQUIRE_FALSE(source.integerPcmFormat().has_value());
    REQUIRE(source.getFormat().getByteFormat()==ByteFormat::FLOAT32);
    std::array<float,10> samples{}; REQUIRE(source.getSamples(samples.data(),10)==10);
    REQUIRE(samples[0]==0.0f); REQUIRE(samples[1]==1.0f/32768.0f);
    REQUIRE(samples[2]==-1.0f/32768.0f); REQUIRE(samples[3]==-1.0f);
}
