#include <QCryptographicHash>
#include <QSharedMemory>
#include <QDir>

#include <base/logger_impl.h>
#include <base/siphash.h>
#include <base/rng.h>
#include <base/uuid.h>
#include <base/assert.h>
#ifdef XAMP_OS_WIN
#include <widget/win32/win32.h>
#endif
#include <widget/str_utilts.h>
#include <singleinstanceapplication.h>

// https://github.com/odzhan/polymutex
static std::string getRandomMutexName(const std::string &src_name) {
	union w128_t {
        uint8_t  b[16];
        uint32_t w[4];
        uint64_t q[2];
        double   d[2];
    } s{};

    PRNG prng;
    s.w[0] = prng.NextInt32() * 0x9e3779b9;
    s.w[1] = prng.NextInt32() * 0x9e3779b9;
    s.q[1] = SipHash::GetHash(s.w[0], s.w[1], src_name);

    Uuid uuid(s.b, s.b + 16);
    std::string str = uuid;

    /*auto parsed_uuid = Uuid::FromString(str);
    const auto *p = reinterpret_cast<const w128_t*>(parsed_uuid.GetBytes().data());
    auto hash = SipHash::GetHash(p->w[0], p->w[1], src_name);

    XAMP_ASSERT(hash == s.q[1]);*/

    return str;
}

SingleInstanceApplication::SingleInstanceApplication() {
}

SingleInstanceApplication::~SingleInstanceApplication() {
#ifdef XAMP_OS_WIN
    if (singular_.is_valid()) {
        ::ReleaseMutex(singular_.get());
        singular_.close();
    }
#endif
}

bool SingleInstanceApplication::attach() const {
#ifdef XAMP_OS_WIN
    if (!win32::isRunning("XAMP2")) {
        const auto name = getRandomMutexName("XAMP2");
        singular_.reset(::CreateMutexA(nullptr, TRUE, name.c_str()));
        if (ERROR_ALREADY_EXISTS == ::GetLastError()) {
            return false;
        }
        XAMP_LOG_DEBUG("My GUID: {}", name);
        return singular_.is_valid();
    }
    return false; 
#endif
}

