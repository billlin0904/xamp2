// Adapted from tobi/disktree mft.rs, commit 158f9cc.
// See ntfs.LICENSE for the original MIT license.
// Parses supplied NTFS buffers; does not open volumes or read audio data.
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <base/base.h>

XAMP_BASE_NAMESPACE_BEGIN

namespace ntfs {
using Bytes = std::span<const uint8_t>;
using Buffer = std::vector<uint8_t>;

// On-disk NTFS identifiers, flags and byte offsets. Offsets are relative to
// the structure named in each constant (not to the containing MFT record).
inline constexpr uint32_t kFileRecordSignature = 0x454c4946;
inline constexpr uint32_t kAttributeStandardInformation = 0x10;
inline constexpr uint32_t kAttributeList = 0x20;
inline constexpr uint32_t kAttributeFileName = 0x30;
inline constexpr uint32_t kAttributeData = 0x80;
inline constexpr uint32_t kAttributeReparsePoint = 0xc0;
inline constexpr uint16_t kRecordInUse = 0x01;
inline constexpr uint16_t kRecordDirectory = 0x02;
inline constexpr uint16_t kAttributeCompressionMask = 0x00ff;
inline constexpr uint16_t kAttributeCompressed = 0x0001;
inline constexpr uint16_t kAttributeEncrypted = 0x4000;
inline constexpr uint16_t kAttributeSparse = 0x8000;
inline constexpr uint32_t kFileAttributeReparsePoint = 0x400;
inline constexpr uint8_t kSizeCodeSignBit = 0x80;
inline constexpr size_t kBootBytesPerSectorOffset = 0x0b;
inline constexpr size_t kBootSectorsPerClusterOffset = 0x0d;
inline constexpr size_t kBootRecordSizeCodeOffset = 0x40;
inline constexpr size_t kBootMftClusterOffset = 0x30;
inline constexpr size_t kBootSectorCountOffset = 0x28;
inline constexpr size_t kRecordSequenceNumberOffset = 0x10;
inline constexpr size_t kRecordFlagsOffset = 0x16;
inline constexpr size_t kRecordBaseReferenceOffset = 0x20;
inline constexpr size_t kRecordUsedSizeOffset = 0x18;
inline constexpr size_t kRecordAttributesOffset = 0x14;
inline constexpr size_t kAttributeFlagsOffset = 0x0c;
inline constexpr size_t kNonResidentStartingVcnOffset = 0x10;
inline constexpr size_t kNonResidentLastVcnOffset = 0x18;
inline constexpr size_t kNonResidentMappingPairsOffset = 0x20;
inline constexpr size_t kNonResidentCompressionUnitOffset = 0x22;
inline constexpr size_t kNonResidentAllocatedSizeOffset = 0x28;
inline constexpr size_t kNonResidentDataSizeOffset = 0x30;
inline constexpr size_t kNonResidentInitializedSizeOffset = 0x38;
inline constexpr size_t kNonResidentCompressedSizeOffset = 0x40;
inline constexpr size_t kResidentValueLengthOffset = 0x10;
inline constexpr size_t kResidentValueOffset = 0x14;
inline constexpr size_t kResidentHeaderSize = 0x18;
inline constexpr size_t kFileNameLengthOffset = 0x40;
inline constexpr size_t kFileNameNamespaceOffset = 0x41;
inline constexpr size_t kFileNameTextOffset = 0x42;
inline constexpr size_t kFileNameAttributesOffset = 0x38;
inline constexpr size_t kFileNameReparseTagOffset = 0x3c;
inline constexpr size_t kStandardInformationAttributesOffset = 0x20;
inline constexpr size_t kAttributeListReferenceOffset = 0x10;

inline constexpr uint64_t kReferenceMask = 0xffffffffffffULL;
XAMP_BASE_API void requireValid(bool ok, const char *message);
XAMP_BASE_API uint64_t checkedAdd(uint64_t left, uint64_t right);
XAMP_BASE_API uint64_t checkedMultiply(uint64_t left, uint64_t right);
XAMP_BASE_API Bytes sliceBytes(Bytes bytes, size_t offset, size_t n);
XAMP_BASE_API uint64_t readInteger(Bytes bytes, size_t offset, size_t n);
XAMP_BASE_API uint16_t readUint16(Bytes bytes, size_t offset);
XAMP_BASE_API uint32_t readUint32(Bytes bytes, size_t offset);
XAMP_BASE_API uint64_t readUint64(Bytes bytes, size_t offset);
XAMP_BASE_API uint32_t recordNumber(uint64_t reference);
XAMP_BASE_API uint16_t sequenceNumber(uint64_t reference);

struct Geometry {
  uint64_t cluster, record, mft, volume;
};

XAMP_BASE_API Geometry parseGeometry(Bytes bytes);

XAMP_BASE_API void applyFixup(std::span<uint8_t> bytes);
// Non-owning view; the source buffer must outlive this attribute.
struct XAMP_BASE_API Attribute {
  Bytes bytes;

  uint32_t kind() const;

  bool isNonResident() const;

  bool isUnnamed() const;

  bool isFirstExtent() const;

  Bytes value() const;

  std::pair<uint64_t, uint64_t> sizes() const;
};

XAMP_BASE_API std::vector<Attribute> parseAttributes(Bytes record);
struct Run {
  uint64_t offset, length;
  bool sparse = false;
};

XAMP_BASE_API std::vector<Run> decodeRuns(Attribute a, uint64_t cluster,
                                          bool allow_sparse = false);
// first borrows storage from the records supplied to parseStream.
struct Stream {
  Attribute first;
  std::vector<Run> runs;
};

XAMP_BASE_API Stream parseStream(const std::vector<Buffer> &records,
                                 uint32_t kind, const Geometry &geometry,
                                 bool allow_sparse = false);

// Owned layout of an unnamed $DATA stream. Records must have their sector
// fixups restored and extension identities checked before parsing.
struct DataStream {
  Buffer resident;
  std::vector<Run> runs;
  uint64_t size = 0;
  uint64_t initialized_size = 0;
  bool nonresident = false;
};

// Compressed/encrypted attributes are rejected for path-based fallback.
XAMP_BASE_API DataStream parseDataStream(const std::vector<Buffer> &records,
                                         const Geometry &geometry);

// The callback must fill the complete destination or throw. It reads offsets
// relative to the volume, never logical file offsets. Sparse/uninitialized
// bytes are zero-filled without calling the volume reader.
using VolumeReader = std::function<void(uint64_t, std::span<uint8_t>)>;
XAMP_BASE_API size_t readData(const DataStream &stream, uint64_t position,
                              std::span<uint8_t> destination,
                              const VolumeReader &read_volume);

struct RecordInfo {
  uint64_t apparent = 0, allocated = 0;
  uint32_t flags = 0, tag = 0;
  uint16_t sequence = 0;
  bool live = false, directory = false;
};

struct FileName {
  uint32_t parent, child;
  uint16_t parent_sequence, child_sequence;
  std::wstring text;
};

struct ExtraRecordInfo {
  uint32_t owner;
  uint16_t sequence;
  std::optional<uint64_t> apparent;
  uint64_t allocated;
  uint32_t tag;
};

struct ParsedRecords {
  std::vector<FileName> names;
  std::vector<ExtraRecordInfo> extra;
  uint64_t bad = 0;
};

XAMP_BASE_API std::optional<FileName> parseFileName(Bytes bytes, uint32_t owner,
                                                    uint16_t sequence);
XAMP_BASE_API void parseRecord(std::span<uint8_t> bytes, uint32_t id,
                               RecordInfo &slot, ParsedRecords &out);
XAMP_BASE_API void mergeExtra(std::vector<RecordInfo> &infos,
                              const ParsedRecords &parsed);
XAMP_BASE_API bool isValidName(const FileName &n,
                               const std::vector<RecordInfo> &infos);
struct ReadRequest {
  uint64_t offset, length, first;
};

XAMP_BASE_API std::vector<ReadRequest> planReads(const std::vector<Run> &runs,
                                                 Bytes bitmap,
                                                 const Geometry &geometry,
                                                 uint64_t count);
} // namespace ntfs

XAMP_BASE_NAMESPACE_END
