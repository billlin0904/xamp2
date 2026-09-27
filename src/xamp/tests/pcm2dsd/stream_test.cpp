#include <stream/pcm2dsdfilestream.h>
#include <stream/avlibfilestream.h>
#include <QTemporaryDir>
#include <QFile>
#include <output_device/win32/asiodsdformat.h>
#include <pcm_dsd_converter.h>
#include <QCoreApplication>
#include <base/logger.h>
#include <base/dll.h>
#include <base/fs.h>
#include <base/pcm.h>
#include <base/dataconverter.h>
#include <stream/api.h>
#include <stream/idspmanager.h>
#include <array>
#include <bit>
#include <cmath>
#include <iostream>
#include <vector>
#include <cstring>
#include <chrono>

using namespace xamp::base;
using namespace xamp::stream;
void require(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
class Source final : public FileStream {
public:
    Source(unsigned bits,unsigned rate,size_t frames) : format_(xamp::pcm::canonical(bits,2,rate)), data_(frames*8) {
        for(size_t i=0;i<frames*2;i++) {
            const uint32_t word=static_cast<uint32_t>(i*2654435761ULL+1) & (UINT32_MAX << (32-bits));
            std::memcpy(data_.data()+i*4,&word,4);
        }
    }
    Uuid getTypeId() const override { return {}; }
    std::string_view getDescription() const override { return "test integer source"; }
    void openFile(const Path&) override { pos_=0; }
    void open(ArchiveEntry) override { pos_=0; }
    void close() override { pos_=data_.size(); }
    bool endOfStream() const override { return pos_==data_.size(); }
    bool isActive() const override { return !endOfStream(); }
    double getDuration() const override { return static_cast<double>(data_.size()/8)/format_.sample_rate; }
    uint32_t getSamples(void* out,uint32_t count) const override {
        const size_t bytes=(std::min)(static_cast<size_t>(count)*4,data_.size()-pos_);
        std::memcpy(out,data_.data()+pos_,bytes);pos_+=bytes;return static_cast<uint32_t>(bytes/4);
    }
    AudioFormat getFormat() const override { return AudioFormat(DataFormat::FORMAT_PCM,2,ByteFormat::SINT32,format_.sample_rate); }
    std::optional<xamp::pcm::Format> integerPcmFormat() const override { return format_; }
    void seek(double seconds) const override { pos_=(std::min)(static_cast<size_t>(std::llround(seconds*format_.sample_rate))*8,data_.size()); }
    uint32_t getSampleSize() const override { return 4; }
    uint32_t getBitDepth() const override { return format_.valid_bits; }
    uint32_t getBitRate() const override { return 0; }
    xamp::pcm::Format format_;
    std::vector<std::byte> data_;
    mutable size_t pos_{};
};
class ForbiddenDsp final : public IAudioProcessor {
public:
    Uuid getTypeId() const override { return {}; }
    std::string_view getDescription() const override { return "forbidden DSP"; }
    void initialize(const Property&) override { throw std::runtime_error("DSP initialized"); }
    bool process(const float*,size_t,BufferRef<float>&) override { throw std::runtime_error("DSP processed DoP"); }
};
std::vector<uint8_t> collect(FileStream& stream,size_t samples) {
    std::vector<std::byte> storage(samples*4);
    std::vector<uint8_t> result;
    auto dsp=StreamFactory::makeDSPManager();
    dsp->addPreDSP(makeAlign<IAudioProcessor,ForbiddenDsp>());
    dsp->addPostDSP(makeAlign<IAudioProcessor,ForbiddenDsp>());
    dsp->setBitPerfect(true); Property config; dsp->initialize(config);
    AudioBuffer<std::byte> fifo(samples*4+1);
    std::vector<std::byte> copy(samples*4);
    while(true) {
        auto block=stream.readPcm(storage);
        if(!block.frames) break;
        dsp->processPcm(block,fifo);
        size_t count=0; fifo.tryRead(copy.data(),block.data.size(),count);
        require(count==block.data.size(),"FIFO count");
        for(size_t i=0;i<count;i++) result.push_back(std::to_integer<uint8_t>(copy[i]));
    }
    require(stream.endOfStream(),"EOF before complete drain");
    return result;
}
std::vector<uint8_t> reference(const Source& source,unsigned mult,double gain,bool dither,const pcm_dsd_options* options=nullptr) {
    pcm_dsd_config cfg;pcm_dsd_default_config(&cfg);
    cfg.input_sample_rate=source.format_.sample_rate;cfg.dsd_multiplier=mult;
    cfg.input_gain=.5*std::pow(10.,gain/20.); cfg.dither_amplitude=dither?std::ldexp(1.,-24):0.;
    pcm_dsd_converter* c=nullptr;require((options?pcm_dsd_create_ex(&cfg,options,&c):pcm_dsd_create(&cfg,&c))==0,"create baseline");
    std::vector<double> pcm(source.data_.size()/4);
    for(size_t i=0;i<pcm.size();i++) pcm[i]=std::bit_cast<int32_t>(xamp::pcm::readSignedWord(source.data_.data()+i*4,source.format_))/std::ldexp(1.,source.format_.valid_bits-1);
    size_t pos=0,used=0,written=0;std::array<uint8_t,514> output;std::vector<uint8_t> bytes;
    while(pos<pcm.size()/2) {
        require(pcm_dsd_process(c,pcm.data()+pos*2,pcm.size()/2-pos,output.data(),output.size(),&used,&written)==0,"process baseline");
        pos+=used;bytes.insert(bytes.end(),output.begin(),output.begin()+written);
    }
    for(;;) {
        const auto status=pcm_dsd_flush(c,output.data(),output.size(),&written);
        bytes.insert(bytes.end(),output.begin(),output.begin()+written);
        if(status==PCM_DSD_FINISHED) break;
        require(status==0,"flush baseline");
    }
    pcm_dsd_destroy(c);return bytes;
}
void testPcmToDouble() {
    namespace pcm = xamp::pcm;
    for (const auto container : {16, 24, 32}) {
        for (int bits = 16; bits <= container; ++bits) {
            for (const auto order : {pcm::ByteOrder::Little, pcm::ByteOrder::Big}) {
                for (const auto alignment : {pcm::Alignment::Left, pcm::Alignment::Right}) {
                    auto format = pcm::canonical(bits, 2, 44100);
                    format.container_bits = container;
                    format.byte_order = order;
                    format.alignment = alignment;
                    std::array<std::byte, 1 + 65 * 4> input{};
                    const std::array<uint32_t, 8> edges{
                        0, 1, UINT32_MAX, 0x80000000, 0x7fffffff,
                        uint32_t{1} << (bits - 1), (uint32_t{1} << (bits - 1)) - 1, 0xaaaaaaaa};
                    for (size_t i = 0; i < 65; ++i) {
                        const uint32_t word = i < edges.size() ? edges[i] : static_cast<uint32_t>(i * 2654435761ULL);
                        for (size_t b = 0; b < format.sampleBytes(); ++b) {
                            const auto shift = 8 * (order == pcm::ByteOrder::Little ? b : format.sampleBytes() - 1 - b);
                            input[1 + i * format.sampleBytes() + b] = static_cast<std::byte>(word >> shift);
                        }
                    }
                    for (size_t count = 0; count <= 65; ++count) {
                        std::array<double, 67> output;
                        output.fill(123.0);
                        convertPcmToDouble(input.data() + 1, output.data() + 1, count, format);
                        require(output.front() == 123.0 && output[count + 1] == 123.0, "PCM double output bounds");
                        for (size_t i = 0; i < count; ++i) {
                            const auto word = pcm::readSignedWord(input.data() + 1 + i * format.sampleBytes(), format);
                            const auto expected = std::bit_cast<int32_t>(word) / std::ldexp(1.0, bits - 1);
                            require(output[i + 1] == expected, "PCM double differs from scalar reference");
                        }
                    }
                }
            }
        }
    }
}

int main(int argc,char** argv) {
    testPcmToDouble();
    QCoreApplication app(argc,argv);
    try {
        XampLoggerFactory.startup();
        {
            // Exercise the real decoder, not only the synthetic FileStream.
            QTemporaryDir directory;
            require(directory.isValid(),"stereo fixture directory");
            const auto path=directory.filePath(QStringLiteral("stereo.wav"));
            QFile wav(path);require(wav.open(QIODevice::WriteOnly),"open stereo fixture");
            QByteArray bytes;
            auto u16=[&](uint16_t v) { bytes.append(static_cast<char>(v));bytes.append(static_cast<char>(v>>8)); };
            auto u32=[&](uint32_t v) { u16(static_cast<uint16_t>(v));u16(static_cast<uint16_t>(v>>16)); };
            bytes.append("RIFF",4);u32(36+257*4);bytes.append("WAVEfmt ",8);u32(16);u16(1);u16(2);
            u32(44100);u32(44100*4);u16(4);u16(16);bytes.append("data",4);u32(257*4);
            for(int i=0;i<257;i++) { u16(static_cast<uint16_t>(1000+i));u16(static_cast<uint16_t>(-2000-i)); }
            require(wav.write(bytes)==bytes.size(),"write stereo fixture");wav.close();
            require(addSharedLibrarySearchDirectory(getComponentsFilePath()),"component DLL directory");
            loadAvLib();
            AvLibFileStream decoded;decoded.setIntegerPcm(true);decoded.openFile(path.toStdWString());
            std::vector<std::byte> storage(257*8);
            const auto block=decoded.readPcm(storage);
            require(block.frames==257 && block.format.channels==2,"decoded stereo frames");
            for(size_t i=0;i<257;i++) {
                require(std::bit_cast<int32_t>(xamp::pcm::readSignedWord(storage.data()+i*8,block.format))==1000+i,"decoded left channel");
                require(std::bit_cast<int32_t>(xamp::pcm::readSignedWord(storage.data()+i*8+4,block.format))==-2000-static_cast<int>(i),"decoded right channel");
            }
            for(auto layout:{xamp::pcm::Layout::Interleaved,xamp::pcm::Layout::Planar}) for(unsigned width:{16u,24u,32u}) {
                auto target=block.format;target.container_bits=static_cast<uint16_t>(width);target.layout=layout;
                std::vector<std::byte> output(257*2*width/8);
                std::array<std::span<std::byte>,2> planes;
                size_t count=1;
                if(layout==xamp::pcm::Layout::Planar) { count=2;planes[0]={output.data(),output.size()/2};planes[1]={output.data()+output.size()/2,output.size()/2}; }
                else planes[0]=output;
                require(xamp::pcm::convert(block,target,{planes.data(),count}),"stereo output conversion");
                for(size_t i=0;i<257;i++) for(size_t ch=0;ch<2;ch++) {
                    const auto index=layout==xamp::pcm::Layout::Planar ? ch*257+i : i*2+ch;
                    const auto value=std::bit_cast<int32_t>(xamp::pcm::readSignedWord(output.data()+index*(width/8),target));
                    require(value==(ch==0 ? 1000+static_cast<int>(i) : -2000-static_cast<int>(i)),"left/right preserved through device format conversion");
                }
            }
            AvLibFileStream floating;floating.openFile(path.toStdWString());std::array<float,514> samples{};
            require(floating.getSamples(samples.data(),514)==514,"float stereo frames");
            for(size_t i=0;i<257;i++) {
                require(std::abs(samples[i*2]-(1000.+i)/32768.)<1e-6,"float left channel");
                require(std::abs(samples[i*2+1]-(-2000.-i)/32768.)<1e-6,"float right channel");
            }
            std::cout<<"PASS: real WAV integer/float decoder and 16/24/32-bit planar/interleaved stereo output\n";
        }

        for(unsigned bits:{16u,24u,32u}) for(unsigned rate:{44100u,48000u,2822400u}) {
            const unsigned mult=64;
            auto owner=makeAlign<FileStream,Source>(bits,rate,777);
            auto expected=reference(*static_cast<Source*>(owner.get()),mult,-3.,true);
            Pcm2DsdFileStream stream(std::move(owner),mult,-3.,true);
            auto data=collect(stream,258);
            require(data.size()==((expected.size()+3)/4)*8,"DoP frame length");
            std::vector<uint8_t> unpacked;
            for(size_t frame=0;frame<data.size()/8;frame++) {
                const size_t i=frame*8;const uint8_t marker=frame%2?0xfa:0x05;
                require(data[i]==0&&data[i+4]==0,"canonical padding");
                require(data[i+3]==marker&&data[i+7]==marker,"DoP stereo markers");
                unpacked.insert(unpacked.end(),{data[i+2],data[i+6],data[i+1],data[i+5]});
            }
            require(std::equal(expected.begin(),expected.end(),unpacked.begin()),"DoP payload differs from DLL");
            stream.seek(0); require(data==collect(stream,2),"read chunk/reset invariance");
            stream.seek(stream.getDuration()); require(collect(stream,2).empty(),"EOF at fractional DoP frame");
        }
        for(unsigned filter:{0u,1u,2u}) for(unsigned modulator:{0u,1u}) for(unsigned block:{64u,256u,1024u}) {
            pcm_dsd_options options{sizeof(pcm_dsd_options),filter,modulator,block};
            auto source=makeAlign<FileStream,Source>(24,48000,3001);
            const auto expected=reference(*static_cast<Source*>(source.get()),64,0.,true,&options);
            Pcm2DsdFileStream converted(std::move(source),64,0.,true,filter,modulator,block);
            const auto data=collect(converted,258);
            std::vector<uint8_t> unpacked;
            for(size_t i=0;i<data.size();i+=8) unpacked.insert(unpacked.end(),{data[i+2],data[i+6],data[i+1],data[i+5]});
            require(unpacked==expected,"selected options reach converter DLL");
            converted.seek(0);require(data==collect(converted,1024),"selected options reset");
        }
        for(unsigned rate:{44100u,48000u,2822400u}) for(size_t frames:{1u,777u,3001u}) {
            auto source=makeAlign<FileStream,Source>(24,rate,frames);
            const auto expected=reference(*static_cast<Source*>(source.get()),64,0.,true);
            Pcm2DsdFileStream native(std::move(source),64,0.,true);
            native.setDSDMode(DsdModes::DSD_MODE_NATIVE);
            require(native.getSampleSize()==1 && !native.integerPcmFormat() && native.supportNativeSD(),"native stream format");
            require(native.getFormat().getSampleRate()==native.getDsdSampleRate(),"native bit clock");
            auto readNative=[&](size_t capacity) {
                std::vector<uint8_t> bytes,buffer(capacity);
                while(auto n=native.getSamples(buffer.data(),static_cast<uint32_t>(capacity)))
                    bytes.insert(bytes.end(),buffer.begin(),buffer.begin()+n);
                require(native.endOfStream(),"native EOF");return bytes;
            };
            require(readNative(514)==expected,"native payload equals C DLL (no DoP headers)");
            native.seek(0);require(readNative(2)==expected,"native tiny reads/reset");
            native.seek(native.getDuration());require(readNative(2).empty(),"native seek EOF");
            native.setDSDMode(DsdModes::DSD_MODE_DOP);
            require(native.integerPcmFormat().has_value() && !collect(native,258).empty(),"native to DoP restart");
        }
        {
            std::array<uint8_t,512> input{};std::array<uint8_t,260> left{},right{};
            for(size_t i=0;i<256;i++) { input[i*2]=static_cast<uint8_t>(i);input[i*2+1]=static_cast<uint8_t>(255-i); }
            for(bool lsb:{false,true}) {
                require(xamp::output_device::win32::copyNativeDsd(input,left,right,lsb),"ASIO planar packing");
                for(size_t i=0;i<256;i++) for(size_t bit=0;bit<8;bit++) {
                    const auto shift=lsb ? 7-bit : bit;
                    require(((left[i]>>shift)&1)==((i>>bit)&1),"ASIO left bit order");
                    require(((right[i]>>shift)&1)==(((255-i)>>bit)&1),"ASIO right bit order");
                }
                require(left[256]==(lsb?0x96:0x69)&&right[259]==left[256],"native idle tail");
            }
            require(!xamp::output_device::win32::copyNativeDsd({input.data(),3},left,right,false),"reject partial stereo frame");
        }
        auto owner=makeAlign<FileStream,Source>(32,44100,10000);
        Pcm2DsdFileStream stream(std::move(owner),64,0.,false);
        stream.seek(.15); const auto first=collect(stream,4096);
        stream.seek(.15); require(first==collect(stream,1024),"seek determinism");
        stream.seek(stream.getDuration());require(collect(stream,1024).empty(),"seek to EOF");
        stream.close();require(stream.endOfStream(),"closed stream");
        if(argc>1 && std::string_view(argv[1])=="--benchmark") {
            for(unsigned mult:{64u,128u,256u}) {
                Pcm2DsdFileStream bench(makeAlign<FileStream,Source>(32,44100,88200),mult,0.,true);
                const auto start=std::chrono::steady_clock::now();
                const auto output=collect(bench,32768);
                const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
                std::cout<<"DSD"<<mult<<": "<<2./seconds<<"x realtime (2 s source, conversion + strict FIFO), "<<output.size()<<" carrier bytes\n";
            }
        }
        std::cout<<"PASS: 16/24/32-bit exact input, DoP ordering/markers, strict DSP bypass, tiny reads, EOF, seek/reset\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
