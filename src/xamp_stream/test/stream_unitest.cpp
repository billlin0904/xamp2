#include "gtest/gtest.h"

#include <stream/bassfilestream.h>

using namespace xamp::stream;

TEST(UnitTest, BassFileGetFormat) {
    BassFileStream file;
    file.OpenFromFile(L"test.flac");
    auto format = file.getFormat();
    EXPECT_TRUE(format.getSampleRate() == 44100);
    EXPECT_TRUE(format.getChannels() == 2);
    EXPECT_TRUE(format.GetInterleavedFormat() == InterleavedFormat::INTERLEAVED);
    EXPECT_TRUE(format.getBitsPerSample() == 32);
    EXPECT_TRUE(format.getByteFormat() == ByteFormat::FLOAT32);
}
