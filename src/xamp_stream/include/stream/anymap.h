//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>

#include <base/fs.h>
#include <base/stl.h>
#include <base/audioformat.h>

#include <any>
#include <string_view>

namespace xamp::stream {

class XAMP_STREAM_API AnyMap {
public:
    template <typename T>
    void AddOrReplace(const std::string_view &name, T&& value) {
        configs_.insert_or_assign(name, std::forward<T>(value));
    }

    AudioFormat AsAudioFormat(const std::string_view& name) const {
        return Get<AudioFormat>(name);
    }

    Path AsPath(const std::string_view& name) const {
        return Get<Path>(name);
    }

    template <typename T>
    T Get(const std::string_view& name) const {
        return std::any_cast<T>(configs_.at(name));
    }

    void Remove(const std::string_view& name) {
        configs_.erase(name);
    }
private:
    OrderedMap<std::string_view, std::any> configs_;
};

}

