#include <stream/bassexception.h>
#include <stream/dsd_utils.h>

namespace xamp::stream {

uint32_t GetDOPSampleRate(uint32_t dsd_speed) {
    switch (dsd_speed) {
        // 32x CD
    case 32:
        return 88200;
        // 64x CD
    case 64:
        return 176400;
        // 128x CD
    case 128:
        return 352800;
        // 256x CD
    case 256:
        return 705600;
    default:
        throw NotSupportFormatException();
    }
}

}
