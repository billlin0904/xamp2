//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifndef XAMP_OS_WIN
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/mman.h>

#include <dlfcn.h>
#include <unistd.h>

#include <base/unique_handle.h>

namespace xamp::base {

struct ModuleHandleTraits final {
    static void* invalid() {
        return nullptr;
    }

    static void close(void* value) {
        ::dlclose(value);
    }
};

struct FileHandleTraits final {
    static int invalid() {
        return -1;
    }

    static void close(int value) {
        ::close(value);
    }
};

struct TimerFdTraits final {
    static int invalid() {
        return -1;
    }

    static void close(int value) {
        ::close(value);
    }
};

using SharedLibraryHandle = UniqueHandle<void*, ModuleHandleTraits>;
using FileHandle = UniqueHandle<int, FileHandleTraits>;
using TimerFdHandle = UniqueHandle<int, TimerFdTraits>;
#endif

}

