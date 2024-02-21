#include "gtest/gtest.h"

#include <stream/bassfilestream.h>

using namespace xamp::stream;

TEST(UnitTest, BassFileGetFormat) {
    BassFileStream file;
    file.OpenFromFile(L"test.flac");
    auto format = file.GetFormat();
    EXPECT_TRUE(format.GetSampleRate() == 44100);
    EXPECT_TRUE(format.GetChannels() == 2);
    EXPECT_TRUE(format.GetInterleavedFormat() == InterleavedFormat::INTERLEAVED);
    EXPECT_TRUE(format.GetBitsPerSample() == 32);
    EXPECT_TRUE(format.GetByteFormat() == ByteFormat::FLOAT32);
}
