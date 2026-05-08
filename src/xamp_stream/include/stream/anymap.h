//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>

#include <base/fs.h>
#include <base/stl.h>
#include <base/audioformat.h>

#include <any>
#include <string_view>

XAMP_STREAM_NAMESPACE_BEGIN

class XAMP_STREAM_API Property {
public:
    template <typename T>
    void Create(const std::string_view &name, T&& value) {
        configs_.insert_or_assign(name, std::forward<T>(value));
    }

    AudioFormat AsAudioFormat(const std::string_view& name) const {
        return Get<AudioFormat>(name);
    }

    Path AsPath(const std::string_view& name) const {
        return Get<Path>(name);
    }

    std::wstring AsStdWString(const std::string_view& name) const {
        return Get<std::wstring>(name);
    }

    template <typename T>
    T Get(const std::string_view& name) const {
        return std::any_cast<T>(configs_.at(name));
    }

    template <typename T>
    bool Set(const std::string_view& name, const T &value) {
	    if (!configs_.contains(name)) {
            return false;
	    }
        configs_[name] = value;
		return true;
    }

    void Remove(const std::string_view& name) {
        configs_.erase(name);
    }
private:
    OrderedMap<std::string_view, std::any> configs_;
};

XAMP_STREAM_NAMESPACE_END

