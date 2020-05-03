//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <vector>
#include <string>
#include <CoreAudio/CoreAudio.h>

namespace xamp::output_device::osx {

template <typename StringType>
static CFStringRef STLStringToCFStringWithEncodingsT(
    const StringType& in,
    CFStringEncoding in_encoding) {
    typename StringType::size_type in_length = in.length();
    if (in_length == 0) {
        return CFSTR("");
    }
    return ::CFStringCreateWithBytes(kCFAllocatorDefault,
                                     reinterpret_cast<const UInt8*>(in.data()),
                                     in_length * sizeof(typename StringType::value_type),
                                     in_encoding,
                                     false);
}

template <typename StringType>
static StringType CFStringToSTLStringWithEncodingT(CFStringRef cfstring,
                                                   CFStringEncoding encoding) {
    auto length = ::CFStringGetLength(cfstring);
    if (length == 0) {
        return StringType();
    }

    auto whole_string = ::CFRangeMake(0, length);
    CFIndex out_size;
    auto converted = ::CFStringGetBytes(cfstring,
                                        whole_string,
                                        encoding,
                                        0,      // lossByte
                                        false,  // isExternalRepresentation
                                        nullptr,   // buffer
                                        0,      // maxBufLen
                                        &out_size);
    if (converted == 0 || out_size == 0) {
        return StringType();
    }

    auto elements = out_size * sizeof(UInt8) / sizeof(typename StringType::value_type) + 1;

    std::vector<typename StringType::value_type> out_buffer(elements);
    converted = ::CFStringGetBytes(cfstring,
                                   whole_string,
                                   encoding,
                                   0,      // lossByte
                                   false,  // isExternalRepresentation
                                   reinterpret_cast<UInt8*>(&out_buffer[0]),
                                   out_size,
                                   nullptr);  // usedBufLen
    if (converted == 0) {
        return StringType();
    }

    out_buffer[elements - 1] = '\0';
    return StringType(&out_buffer[0], elements - 1);
}

inline std::string SysCFStringRefToUTF8(CFStringRef ref) {
    return CFStringToSTLStringWithEncodingT<std::string>(ref,
                                                         kCFStringEncodingUTF8);
}

inline std::wstring SysCFStringRefToWide(CFStringRef ref) {
    return CFStringToSTLStringWithEncodingT<std::wstring>(ref,
                                                          kCFStringEncodingUTF32LE);
}

inline CFStringRef SysUTF8ToCFStringRef(const std::string& utf8) {
    return STLStringToCFStringWithEncodingsT(utf8, kCFStringEncodingUTF8);
}

inline CFStringRef SysWideToCFStringRef(const std::wstring& wide) {
    return STLStringToCFStringWithEncodingsT(wide, kCFStringEncodingUTF32LE);
}

}

