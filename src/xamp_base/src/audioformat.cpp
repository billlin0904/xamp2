#include <base/audioformat.h>

namespace xamp::base {

const AudioFormat AudioFormat::UnknowFormat;
const AudioFormat AudioFormat::PCM48Khz(DataFormat::FORMAT_PCM,
	2, ByteFormat::SINT16, 48000);

}
