#include <base/ntfs.h>
#include <base/ntfsfilestream.h>
#include <base/platfrom_handle.h>

#include <algorithm>
#include <bit>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <system_error>
#include <unordered_set>

#ifdef XAMP_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <base/unique_handle.h>
#include <winioctl.h>
#endif

XAMP_BASE_NAMESPACE_BEGIN

#ifdef XAMP_OS_WIN

namespace {

using NtfsHandle = FileHandle;

void checkWindows(BOOL result, const char *action) {
  if (!result) {
    auto last_error = ::GetLastError();
    throw std::system_error(static_cast<int>(last_error),
                            std::system_category(), action);
  }
}

uint64_t referenceOf(HANDLE file) {
  BY_HANDLE_FILE_INFORMATION info{};
  checkWindows(::GetFileInformationByHandle(file, &info),
               "Query NTFS file identity");
  ntfs::requireValid(!(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY),
                     "Expected an NTFS file");
  return (uint64_t(info.nFileIndexHigh) << 32) | info.nFileIndexLow;
}

std::wstring volumeOf(HANDLE file) {
  const DWORD flags = FILE_NAME_NORMALIZED | VOLUME_NAME_GUID;
  const auto required = ::GetFinalPathNameByHandleW(file, nullptr, 0, flags);
  checkWindows(required != 0, "Resolve NTFS volume");
  std::wstring name(required, L'\0');
  const auto length =
      ::GetFinalPathNameByHandleW(file, name.data(), required, flags);
  checkWindows(length != 0, "Resolve NTFS volume");
  ntfs::requireValid(length < required,
                     "NTFS path changed while resolving its volume");
  name.resize(length);
  ntfs::requireValid(name.starts_with(L"\\\\?\\Volume{"),
                     "Expected a local NTFS volume");
  const auto end = name.find(L"}\\");
  ntfs::requireValid(end != std::wstring::npos, "Invalid NTFS volume path");
  // This reader selects the unnamed stream; never silently drop an ADS.
  ntfs::requireValid(name.find(L':', end + 2) == std::wstring::npos,
                     "Named NTFS streams require path-based I/O");
  return name.substr(0, end + 1);
}

ntfs::Buffer readRecord(HANDLE volume, uint64_t reference,
                        const ntfs::Geometry &geometry) {
  DWORD returned = 0;
  const auto header_size =
      offsetof(NTFS_FILE_RECORD_OUTPUT_BUFFER, FileRecordBuffer);
  ntfs::Buffer output(sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER) + geometry.record -
                      1);
  NTFS_FILE_RECORD_INPUT_BUFFER input{};
  input.FileReferenceNumber.QuadPart = std::bit_cast<int64_t>(reference);
  checkWindows(::DeviceIoControl(volume, FSCTL_GET_NTFS_FILE_RECORD, &input,
                                 sizeof(input), output.data(),
                                 static_cast<DWORD>(output.size()), &returned,
                                 nullptr),
               "Read NTFS MFT record");
  ntfs::requireValid(returned >= header_size && returned <= output.size(),
                     "Truncated MFT response");
  const auto returned_reference = ntfs::readUint64(output, 0);
  const auto record_length = ntfs::readUint32(output, 8);
  // This FSCTL may return an earlier live record rather than the requested ID.
  ntfs::requireValid((returned_reference & ntfs::kReferenceMask) ==
                         (reference & ntfs::kReferenceMask),
                     "MFT response belongs to another file");
  ntfs::requireValid(record_length == geometry.record &&
                         record_length <= returned - header_size,
                     "Truncated MFT record");
  auto record = std::span<uint8_t>(output).subspan(header_size, record_length);
  ntfs::requireValid(
      ntfs::readUint32(record, 0) == ntfs::kFileRecordSignature &&
          (ntfs::readUint16(record, ntfs::kRecordFlagsOffset) &
           (ntfs::kRecordInUse | ntfs::kRecordDirectory)) ==
              ntfs::kRecordInUse &&
          ntfs::readUint16(record, ntfs::kRecordSequenceNumberOffset) ==
              ntfs::sequenceNumber(reference),
      "Stale NTFS file reference");
  // Accept raw sector fixups or the already restored form returned by NTFS.
  const auto usa_offset = ntfs::readUint16(record, 4);
  const auto usa_count = ntfs::readUint16(record, 6);
  ntfs::requireValid(usa_count == record.size() / 512 + 1 && usa_offset >= 8 &&
                         usa_offset + size_t(usa_count) * 2 <= 510,
                     "Invalid MFT update sequence array");
  bool raw = true;
  bool restored = true;
  for (size_t i = 1; i < usa_count; ++i) {
    const auto tail = ntfs::readUint16(record, i * 512 - 2);
    raw &= tail == ntfs::readUint16(record, usa_offset);
    restored &= tail == ntfs::readUint16(record, usa_offset + i * 2);
  }
  if (raw) {
    ntfs::applyFixup(record);
  } else {
    ntfs::requireValid(restored, "Torn MFT record");
  }
  (void)ntfs::parseAttributes(record);
  return {record.begin(), record.end()};
}
} // namespace

class NtfsFileStream::Impl {
public:
  explicit Impl(const Path &path) {
    // Retain read access and deny writes/deletion while using a physical
    // layout. This also checks the file ACL before privileged volume access is
    // attempted.
    original_.reset(::CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                                  nullptr, OPEN_EXISTING, 0, nullptr));
    checkWindows(bool(original_), "Open NTFS file identity");
    // A local volume GUID does not imply NTFS (ReFS/exFAT use GUIDs too).
    // Query the opened file so junctions/mount points resolve to the actual
    // backing file system, rather than inferring it from the drive letter.
    wchar_t file_system[MAX_PATH + 1]{};
    checkWindows(::GetVolumeInformationByHandleW(original_.get(), nullptr, 0,
                                                 nullptr, nullptr, nullptr,
                                                 file_system, MAX_PATH + 1),
                 "Query backing file system");
    if (::_wcsicmp(file_system, L"NTFS") != 0) {
      throw std::system_error(
          ERROR_NOT_SUPPORTED, std::system_category(),
          "Raw NTFS reader requires NTFS; actual file system=" +
              Path(file_system).string());
    }
    reference_ = referenceOf(original_.get());
    BY_HANDLE_FILE_INFORMATION info{};
    checkWindows(::GetFileInformationByHandle(original_.get(), &info),
                 "Query NTFS file attributes");
    ntfs::requireValid(
        !(info.dwFileAttributes &
          (FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_ENCRYPTED |
           FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_OFFLINE)),
        "NTFS file attributes require fallback");
    const auto volume_name = volumeOf(original_.get());
    volume_.reset(
        ::CreateFileW(volume_name.c_str(), GENERIC_READ,
                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                      nullptr, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, nullptr));
    checkWindows(bool(volume_), "Open NTFS volume for raw access");

    NTFS_VOLUME_DATA_BUFFER volume_data{};
    DWORD returned = 0;
    const auto geometry_action =
        "FSCTL_GET_NTFS_VOLUME_DATA (file system=NTFS, volume=" +
        Path(volume_name).string() + ")";
    checkWindows(::DeviceIoControl(volume_.get(), FSCTL_GET_NTFS_VOLUME_DATA,
                                   nullptr, 0, &volume_data,
                                   sizeof(volume_data), &returned, nullptr),
                 geometry_action.c_str());
    ntfs::requireValid(returned >= sizeof(volume_data) &&
                           volume_data.NumberSectors.QuadPart > 0,
                       "Invalid NTFS volume geometry");
    sector_size_ = volume_data.BytesPerSector;
    ntfs::requireValid(std::has_single_bit(sector_size_) &&
                           sector_size_ >= 512 && sector_size_ <= 4096,
                       "Unsupported NTFS sector size");
    geometry_.volume = ntfs::checkedMultiply(
        uint64_t(volume_data.NumberSectors.QuadPart), sector_size_);

    // Keep the data runs stable even if a defragmenter is active. If the OS
    // cannot protect them, do not read from potentially stale physical offsets.
    MARK_HANDLE_INFO mark{};
    mark.VolumeHandle = volume_.get();
    mark.HandleInfo = MARK_HANDLE_PROTECT_CLUSTERS;
    checkWindows(::DeviceIoControl(original_.get(), FSCTL_MARK_HANDLE, &mark,
                                   sizeof(mark), nullptr, 0, &returned,
                                   nullptr),
                 "Protect NTFS data clusters");
    buffer_.reset(::VirtualAlloc(nullptr, kReadSize + 4096,
                                 MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    checkWindows(bool(buffer_), "Allocate aligned NTFS read buffer");

    // The first DASD read performs NTFS's normal volume coherency flush. Do
    // not use MARK_HANDLE_SUPPRESS_VOLUME_OPEN_FLUSH on this handle.
    ntfs::Buffer boot(512);
    readVolume(0, boot);
    const auto boot_geometry = ntfs::parseGeometry(boot);
    ntfs::requireValid(boot_geometry.cluster == volume_data.BytesPerCluster &&
                           boot_geometry.record ==
                               volume_data.BytesPerFileRecordSegment,
                       "Inconsistent NTFS geometry");
    geometry_.cluster = boot_geometry.cluster;
    geometry_.record = boot_geometry.record;
    geometry_.mft = boot_geometry.mft;

    auto records = readRecords();
    data_ = ntfs::parseDataStream(records, geometry_);
    const uint64_t file_size =
        (uint64_t(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
    ntfs::requireValid(data_.size == file_size,
                       "NTFS data size changed during open");
  }

  void readVolume(uint64_t offset, std::span<uint8_t> destination) {
    ntfs::requireValid(offset <= geometry_.volume &&
                           destination.size() <= geometry_.volume - offset,
                       "NTFS read outside volume");
    while (!destination.empty()) {
      const auto count = (std::min)(destination.size(), kReadSize);
      const auto start = offset - offset % sector_size_;
      const auto prefix = static_cast<size_t>(offset - start);
      const auto aligned_length =
          ((prefix + count + sector_size_ - 1) / sector_size_) * sector_size_;
      ntfs::requireValid(start <= INT64_MAX &&
                             aligned_length <= geometry_.volume - start,
                         "Aligned NTFS read outside volume");
      LARGE_INTEGER position{};
      position.QuadPart = static_cast<int64_t>(start);
      checkWindows(
          ::SetFilePointerEx(volume_.get(), position, nullptr, FILE_BEGIN),
          "Seek NTFS volume");
      DWORD received = 0;
      checkWindows(::ReadFile(volume_.get(), buffer_.get(),
                              static_cast<DWORD>(aligned_length), &received,
                              nullptr),
                   "Read NTFS volume data");
      ntfs::requireValid(received == aligned_length, "Short NTFS volume read");
      std::memcpy(destination.data(),
                  static_cast<const uint8_t *>(buffer_.get()) + prefix, count);
      destination = destination.subspan(count);
      offset += count;
    }
  }

  std::vector<ntfs::Buffer> readRecords() {
    std::vector<ntfs::Buffer> records;
    records.push_back(readRecord(volume_.get(), reference_, geometry_));
    ntfs::requireValid(ntfs::readUint64(records.front(),
                                        ntfs::kRecordBaseReferenceOffset) == 0,
                       "Expected base MFT record");
    bool has_list = false;
    for (const auto &attribute : ntfs::parseAttributes(records.front())) {
      has_list |= attribute.kind() == ntfs::kAttributeList;
    }
    if (!has_list) {
      return records;
    }
    const auto stream =
        ntfs::parseStream(records, ntfs::kAttributeList, geometry_);
    ntfs::Buffer list;
    if (!stream.first.isNonResident()) {
      const auto value = stream.first.value();
      list.assign(value.begin(), value.end());
    } else {
      const auto length = ntfs::readUint64(stream.first.bytes,
                                           ntfs::kNonResidentDataSizeOffset);
      ntfs::requireValid(
          length <= (4u << 20) &&
              ntfs::readUint64(stream.first.bytes,
                               ntfs::kNonResidentInitializedSizeOffset) >=
                  length,
          "Unsupported NTFS attribute list size");
      ntfs::DataStream list_stream;
      list_stream.nonresident = true;
      list_stream.size = list_stream.initialized_size = length;
      list_stream.runs = stream.runs;
      list.resize(static_cast<size_t>(length));
      ntfs::readData(list_stream, 0, list,
                     [this](uint64_t offset, std::span<uint8_t> bytes) {
                       readVolume(offset, bytes);
                     });
    }
    std::unordered_set<uint64_t> seen{reference_};
    for (size_t offset = 0; offset < list.size();) {
      const auto kind = ntfs::readUint32(list, offset);
      if (kind == 0 || kind == UINT32_MAX) {
        break;
      }
      const auto length = ntfs::readUint16(list, offset + 4);
      ntfs::requireValid(length >= 26, "Invalid NTFS attribute list entry");
      const auto entry = ntfs::sliceBytes(list, offset, length);
      if (kind == ntfs::kAttributeData && entry[6] == 0) {
        const auto reference =
            ntfs::readUint64(entry, ntfs::kAttributeListReferenceOffset);
        if (seen.insert(reference).second) {
          ntfs::requireValid(records.size() < 256,
                             "Too many NTFS extension records");
          auto record = readRecord(volume_.get(), reference, geometry_);
          ntfs::requireValid(
              ntfs::readUint64(record, ntfs::kRecordBaseReferenceOffset) ==
                  reference_,
              "Stale MFT extension owner");
          records.push_back(std::move(record));
        }
      }
      offset += length;
    }
    return records;
  }

  struct VirtualDeleter {
    void operator()(void *pointer) const {
      ::VirtualFree(pointer, 0, MEM_RELEASE);
    }
  };
  static constexpr size_t kReadSize = 1u << 20;
  NtfsHandle original_;
  NtfsHandle volume_;
  std::unique_ptr<void, VirtualDeleter> buffer_;
  ntfs::Geometry geometry_{};
  ntfs::DataStream data_;
  uint64_t reference_ = 0;
  uint64_t position_ = 0;
  uint32_t sector_size_ = 0;
};

NtfsFileStream::NtfsFileStream(const Path& path)
    : impl_(std::make_unique<Impl>(path)) {
}

size_t NtfsFileStream::read(void *buffer, size_t length) {
  if (length == 0) {
    return 0;
  }
  ntfs::requireValid(buffer != nullptr, "Null NTFS read buffer");
  const auto received = ntfs::readData(
      impl_->data_, impl_->position_, {static_cast<uint8_t *>(buffer), length},
      [this](uint64_t offset, std::span<uint8_t> bytes) {
        impl_->readVolume(offset, bytes);
      });
  impl_->position_ += received;
  return received;
}

void NtfsFileStream::seek(int64_t offset, int origin) {
  uint64_t base = 0;
  switch (origin) {
  case SEEK_SET:
    break;
  case SEEK_CUR:
    base = impl_->position_;
    break;
  case SEEK_END:
    base = impl_->data_.size;
    break;
  default:
    throw std::invalid_argument("Invalid NTFS seek origin");
  }
  uint64_t position;
  if (offset < 0) {
    const auto distance = uint64_t(-(offset + 1)) + 1;
    ntfs::requireValid(distance <= base, "NTFS seek before beginning");
    position = base - distance;
  } else {
    position = ntfs::checkedAdd(base, static_cast<uint64_t>(offset));
  }
  ntfs::requireValid(position <= INT64_MAX, "NTFS seek overflow");
  impl_->position_ = position;
}

uint64_t NtfsFileStream::tell() const {
    return impl_->position_;
}

uint64_t NtfsFileStream::size() const {
    return impl_->data_.size;
}

uint64_t NtfsFileStream::fileReference() const {
    return impl_->reference_;
}

#else
class NtfsFileStream::Impl {};

NtfsFileStream::NtfsFileStream(const Path&) {
  throw std::runtime_error("NTFS file streams require Windows");
}
size_t NtfsFileStream::read(void *, size_t) {
  throw std::runtime_error("NTFS is unavailable");
}
void NtfsFileStream::seek(int64_t, int) {
  throw std::runtime_error("NTFS is unavailable");
}
uint64_t NtfsFileStream::tell() const {
  throw std::runtime_error("NTFS is unavailable");
}
uint64_t NtfsFileStream::size() const {
  throw std::runtime_error("NTFS is unavailable");
}
uint64_t NtfsFileStream::fileReference() const {
  throw std::runtime_error("NTFS is unavailable");
}
#endif

NtfsFileStream::~NtfsFileStream() = default;

XAMP_BASE_NAMESPACE_END
