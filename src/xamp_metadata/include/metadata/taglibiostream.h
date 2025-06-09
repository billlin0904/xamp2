//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <metadata/taglib.h>
#include <base/fs.h>
#include <base/stl.h>
#include <base/memory.h>

XAMP_METADATA_NAMESPACE_BEGIN

class TaglibIOStream : public TagLib::IOStream {
public:
    explicit TaglibIOStream(const Path& p, bool read_only = false)
        : fast_(p, read_only ? FastIOStream::Mode::Read
            : FastIOStream::Mode::ReadWrite) {
    }

    TagLib::FileName name() const override {
        return fast_.path().wstring().c_str();
    }

    TagLib::ByteVector readBlock(size_t len) override {
        //std::vector<char> buf(len);
        //auto n = fast_.read(buf.data(), len);
        //return TagLib::ByteVector(buf.data(), static_cast<uint32_t>(n));
        TagLib::ByteVector buf;
        buf.resize(static_cast<uint32_t>(len));
        auto n = fast_.read(buf.data(), len);
        buf.resize(static_cast<uint32_t>(n));
        return buf;
    }

    void writeBlock(const TagLib::ByteVector& v) override {
        fast_.write(v.data(), v.size());
    }

    void insert(const TagLib::ByteVector& data, offset_t start = 0, size_t replace = 0) override {
        seek(start, Beginning);
        if (replace) 
            removeBlock(start, replace);
        writeBlock(data);
    }

    void removeBlock(offset_t start = 0, size_t len = 0) override {
        if (len == 0) 
            return;

        const auto total = length();
        if (start + len >= total) {
            truncate(start); return; 
        }

        std::vector<char> tail(static_cast<size_t>(total - (start + len)));
        seek(start + len, Beginning);
        auto tail_vec = readBlock(tail.size());

        seek(start, Beginning);
        writeBlock(tail_vec);
        truncate(total - len);
    }

    bool readOnly() const override { 
        return fast_.read_only(); 
    }

    bool isOpen()  const override { 
        return fast_.is_open();
    }

    long long length() override { 
        return static_cast<long long>(fast_.size());
    }

    void seek(long long off, Position p = Beginning) override {
        fast_.seek(off,
            p == Beginning ? SEEK_SET :
            p == Current ? SEEK_CUR : SEEK_END);
    }

    long long tell() const override { 
        return static_cast<long long>(fast_.tell());
    }

    void truncate(long long l) override { 
        fast_.truncate(static_cast<uint64_t>(l)); 
    }

private:
    FastIOStream fast_;
};

XAMP_METADATA_NAMESPACE_END


