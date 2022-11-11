#include <QCryptographicHash>
#include <QSharedMemory>
#include <QDir>

#include <base/logger_impl.h>
#include <base/rng.h>
#include <base/assert.h>

#ifdef XAMP_OS_WIN
#include <widget/win32/win32.h>
#endif

#include <widget/str_utilts.h>
#include <singleinstanceapplication.h>

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
    const auto name = "XAMP2_" + win32::getRandomMutexName("XAMP2");
	/*if (!win32::isRunning("XAMP2")) {
        singular_.reset(::CreateMutexA(nullptr, TRUE, name.c_str()));
        if (ERROR_ALREADY_EXISTS == ::GetLastError()) {
            return false;
        }
        XAMP_LOG_DEBUG("My GUID: {} Is running: {}", name, win32::isRunning("XAMP2"));
        return singular_.is_valid();
    }
    return false;*/
    singular_.reset(::CreateMutexA(nullptr, TRUE, name.c_str()));
    if (ERROR_ALREADY_EXISTS == ::GetLastError()) {
        return false;
    }
    XAMP_LOG_DEBUG("My GUID: {}", name);
    return singular_.is_valid();
#endif
}

