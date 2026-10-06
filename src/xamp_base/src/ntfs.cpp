// Adapted from tobi/disktree mft.rs, commit 158f9cc.
// See ../include/base/ntfs.LICENSE for the original MIT license.

#include <base/ntfs.h>

#include <algorithm>
#include <bit>
#include <stdexcept>

XAMP_BASE_NAMESPACE_BEGIN

namespace ntfs {

void requireValid(bool ok, const char *message) {
  if (!ok) {
    throw std::runtime_error(message);
  }
}

uint64_t checkedAdd(uint64_t left, uint64_t right) {
  requireValid(right <= UINT64_MAX - left, "NTFS integer overflow");
  return left + right;
}

uint64_t checkedMultiply(uint64_t left, uint64_t right) {
  requireValid(left == 0 || right <= UINT64_MAX / left,
               "NTFS integer overflow");
  return left * right;
}

Bytes sliceBytes(Bytes bytes, size_t offset, size_t n) {
  requireValid(offset <= bytes.size() && n <= bytes.size() - offset,
               "Truncated NTFS field");
  return bytes.subspan(offset, n);
}

uint64_t readInteger(Bytes bytes, size_t offset, size_t n) {
  requireValid(n > 0 && n <= 8, "Invalid integer width");
  auto v = sliceBytes(bytes, offset, n);
  uint64_t out = 0;
  for (size_t i = 0; i < n; ++i) {
    out |= uint64_t(v[i]) << (i * 8);
  }
  return out;
}

uint16_t readUint16(Bytes bytes, size_t offset) {
  return uint16_t(readInteger(bytes, offset, 2));
}

uint32_t readUint32(Bytes bytes, size_t offset) {
  return uint32_t(readInteger(bytes, offset, 4));
}

uint64_t readUint64(Bytes bytes, size_t offset) {
  return readInteger(bytes, offset, 8);
}

uint32_t recordNumber(uint64_t reference) {
  requireValid((reference & kReferenceMask) <= UINT32_MAX,
               "MFT reference exceeds 32 bits");
  return uint32_t(reference & kReferenceMask);
}

uint16_t sequenceNumber(uint64_t reference) {
  return uint16_t(reference >> 48);
}

Geometry parseGeometry(Bytes bytes) {
  const auto signature = sliceBytes(bytes, 3, 8);
  requireValid(std::string(signature.begin(), signature.end()) == "NTFS    ",
               "Not NTFS");
  uint64_t sector = readUint16(bytes, kBootBytesPerSectorOffset);
  uint8_t count = sliceBytes(bytes, kBootSectorsPerClusterOffset, 1)[0];
  requireValid(count <= kSizeCodeSignBit || 256u - count < 64,
               "Invalid cluster exponent");
  uint64_t clusters =
      count > kSizeCodeSignBit ? uint64_t(1) << (256u - count) : count;
  uint64_t cluster = checkedMultiply(sector, clusters);
  uint8_t record_size_code = sliceBytes(bytes, kBootRecordSizeCodeOffset, 1)[0];
  requireValid(record_size_code < kSizeCodeSignBit ||
                   256u - record_size_code < 64,
               "Invalid record exponent");
  uint64_t record = record_size_code >= kSizeCodeSignBit
                        ? uint64_t(1) << (256u - record_size_code)
                        : checkedMultiply(cluster, record_size_code);
  requireValid(std::has_single_bit(sector) && sector >= 512 && sector <= 4096 &&
                   std::has_single_bit(cluster) && cluster <= (2u << 20) &&
                   std::has_single_bit(record) && record >= 1024 &&
                   record <= 4096,
               "Unsupported NTFS geometry");
  Geometry geometry{
      cluster, record,
      checkedMultiply(readUint64(bytes, kBootMftClusterOffset), cluster),
      checkedMultiply(readUint64(bytes, kBootSectorCountOffset), sector)};

  requireValid(checkedAdd(geometry.mft, record) <= geometry.volume,
               "MFT is outside volume");
  return geometry;
}

void applyFixup(std::span<uint8_t> bytes) {
  requireValid(bytes.size() >= 48 && bytes.size() % 512 == 0,
               "Invalid FILE record size");
  requireValid(readUint32(bytes, 0) == kFileRecordSignature,
               "Missing FILE signature");
  size_t offset = readUint16(bytes, 4), count = readUint16(bytes, 6);
  requireValid(count == bytes.size() / 512 + 1 && offset >= 8 &&
                   offset + count * 2 <= 510,
               "Invalid update sequence array");
  auto check = readUint16(bytes, offset);
  // Check all sectors before changing anything, so a torn record is rejected.
  for (size_t i = 1; i < count; ++i) {
    requireValid(readUint16(bytes, i * 512 - 2) == check, "Torn FILE record");
  }
  for (size_t i = 1; i < count; ++i) {
    bytes[i * 512 - 2] = bytes[offset + i * 2];
    bytes[i * 512 - 1] = bytes[offset + i * 2 + 1];
  }
}

std::vector<Attribute> parseAttributes(Bytes record) {
  auto used = readUint32(record, kRecordUsedSizeOffset);
  requireValid(used <= record.size() && used >= 48, "Invalid FILE used length");
  record = record.first(used);
  size_t offset = readUint16(record, kRecordAttributesOffset);
  requireValid(offset >= 48 && offset % 8 == 0, "Invalid attribute offset");
  std::vector<Attribute> out;
  for (;;) {
    if (readUint32(record, offset) == UINT32_MAX) {
      return out;
    }
    size_t length = readUint32(record, offset + 4);
    requireValid(length >= 24 && length % 8 == 0, "Invalid attribute length");
    auto bytes = sliceBytes(record, offset, length);
    requireValid(bytes[8] <= 1 && (bytes[8] == 0 || length >= 64),
                 "Invalid attribute header");
    if (bytes[9]) {
      sliceBytes(bytes, readUint16(bytes, 10), size_t(bytes[9]) * 2);
    }
    out.push_back({bytes});
    offset += length;
  }
}

std::vector<Run> decodeRuns(Attribute a, uint64_t cluster, bool allow_sparse) {
  requireValid(a.isNonResident(), "Expected nonresident stream");
  requireValid(std::has_single_bit(cluster), "Invalid cluster size");
  size_t offset = readUint16(a.bytes, kNonResidentMappingPairsOffset);
  requireValid(offset >= 64, "Invalid mapping-pairs offset");
  int64_t logical_cluster_number = 0;
  std::vector<Run> out;
  for (;;) {
    uint8_t header = sliceBytes(a.bytes, offset++, 1)[0];
    if (!header) {
      return out;
    }
    size_t length_width = header & 15, offset_width = header >> 4;
    requireValid(length_width >= 1 && length_width <= 8 && offset_width <= 8 &&
                     (offset_width != 0 || allow_sparse),
                 "Sparse/invalid MFT run");
    uint64_t count = readInteger(a.bytes, offset, length_width);
    offset += length_width;
    requireValid(count > 0, "Empty NTFS run");
    if (offset_width == 0) {
      out.push_back({0, checkedMultiply(count, cluster), true});
      continue;
    }
    uint64_t bits = readInteger(a.bytes, offset, offset_width);
    if (offset_width < 8 && (bits & (uint64_t(1) << (offset_width * 8 - 1)))) {
      bits |= UINT64_MAX << (offset_width * 8);
    }
    int64_t delta = std::bit_cast<int64_t>(bits);
    requireValid((delta >= 0 && logical_cluster_number <= INT64_MAX - delta) ||
                     (delta < 0 && delta >= -logical_cluster_number),
                 "Invalid relative MFT run");
    logical_cluster_number += delta;
    offset += offset_width;
    requireValid(count > 0, "Empty MFT run");
    Run r{checkedMultiply(uint64_t(logical_cluster_number), cluster),
          checkedMultiply(count, cluster)};

    checkedAdd(r.offset, r.length);
    out.push_back(r);
  }
}

Stream parseStream(const std::vector<Buffer> &records, uint32_t kind,
                   const Geometry &geometry, bool allow_sparse) {
  std::vector<Attribute> pieces;
  for (auto &record : records) {
    for (auto a : parseAttributes(record)) {
      if (a.kind() == kind && a.isUnnamed()) {
        pieces.push_back(a);
      }
    }
  }
  requireValid(!pieces.empty(), "Missing MFT stream");
  std::sort(pieces.begin(), pieces.end(), [](auto left, auto right) {
    return (left.isNonResident()
                ? readUint64(left.bytes, kNonResidentStartingVcnOffset)
                : 0) <
           (right.isNonResident()
                ? readUint64(right.bytes, kNonResidentStartingVcnOffset)
                : 0);
  });
  Stream out{pieces.front(), {}};

  uint64_t virtual_cluster_number = 0;
  for (auto a : pieces) {
    if (!a.isNonResident()) {
      requireValid(pieces.size() == 1, "Mixed resident/nonresident MFT stream");
      return out;
    }
    requireValid(readUint64(a.bytes, kNonResidentStartingVcnOffset) ==
                     virtual_cluster_number,
                 "Missing/overlapping MFT extent");
    for (auto r : decodeRuns(a, geometry.cluster, allow_sparse)) {
      requireValid(r.sparse ||
                       checkedAdd(r.offset, r.length) <= geometry.volume,
                   "Run outside volume");
      virtual_cluster_number =
          checkedAdd(virtual_cluster_number, r.length / geometry.cluster);
      out.runs.push_back(r);
    }
    requireValid(virtual_cluster_number != 0 &&
                     readUint64(a.bytes, kNonResidentLastVcnOffset) ==
                         virtual_cluster_number - 1,
                 "Invalid extent VCN range");
  }
  return out;
}

DataStream parseDataStream(const std::vector<Buffer> &records,
                           const Geometry &geometry) {
  for (const auto &record : records) {
    for (const auto &attribute : parseAttributes(record)) {
      if (attribute.kind() == kAttributeData && attribute.isUnnamed()) {
        const auto flags = readUint16(attribute.bytes, kAttributeFlagsOffset);
        requireValid(
            (flags & (kAttributeCompressionMask | kAttributeEncrypted)) == 0,
            "Compressed/encrypted NTFS data requires fallback");
        if (attribute.isNonResident()) {
          requireValid(readUint16(attribute.bytes,
                                  kNonResidentCompressionUnitOffset) == 0,
                       "Compressed NTFS data requires fallback");
        }
      }
    }
  }
  const auto parsed = parseStream(records, kAttributeData, geometry, true);
  DataStream data;
  data.nonresident = parsed.first.isNonResident();
  if (!data.nonresident) {
    const auto bytes = parsed.first.value();
    data.resident.assign(bytes.begin(), bytes.end());
    data.size = data.initialized_size = bytes.size();
    return data;
  }
  data.size = readUint64(parsed.first.bytes, kNonResidentDataSizeOffset);
  data.initialized_size =
      readUint64(parsed.first.bytes, kNonResidentInitializedSizeOffset);
  requireValid(data.size <= INT64_MAX && data.initialized_size <= data.size,
               "Invalid NTFS data size");
  data.runs = parsed.runs;
  uint64_t covered = 0;
  for (const auto &run : data.runs) {
    requireValid(!run.sparse ||
                     (readUint16(parsed.first.bytes, kAttributeFlagsOffset) &
                      kAttributeSparse),
                 "Sparse run in a non-sparse data stream");
    covered = checkedAdd(covered, run.length);
  }
  requireValid(covered >= data.size, "NTFS runs do not cover the data stream");
  return data;
}

size_t readData(const DataStream &stream, uint64_t position,
                std::span<uint8_t> destination,
                const VolumeReader &read_volume) {
  if (position >= stream.size || destination.empty()) {
    return 0;
  }
  const auto length = static_cast<size_t>(
      std::min(uint64_t(destination.size()), stream.size - position));
  destination = destination.first(length);
  if (!stream.nonresident) {
    const auto bytes =
        sliceBytes(stream.resident, static_cast<size_t>(position), length);
    std::copy(bytes.begin(), bytes.end(), destination.begin());
    return length;
  }
  // Never expose allocated but uninitialized disk bytes.
  std::fill(destination.begin(), destination.end(), uint8_t{0});
  if (position >= stream.initialized_size) {
    return length;
  }
  const auto initialized =
      std::min(uint64_t(length), stream.initialized_size - position);
  uint64_t logical_start = 0;
  size_t copied = 0;
  for (const auto &run : stream.runs) {
    const auto logical_end = checkedAdd(logical_start, run.length);
    if (position < logical_end && copied < initialized) {
      const auto within_run = position - logical_start;
      const auto count = static_cast<size_t>(
          std::min(run.length - within_run, initialized - copied));
      if (!run.sparse) {
        requireValid(bool(read_volume), "Missing NTFS volume reader");
        read_volume(checkedAdd(run.offset, within_run),
                    destination.subspan(copied, count));
      }
      copied += count;
      position += count;
    }
    logical_start = logical_end;
    if (copied == initialized) {
      break;
    }
  }
  requireValid(copied == initialized, "Incomplete NTFS data mapping");
  return length;
}

std::optional<FileName> parseFileName(Bytes bytes, uint32_t owner,
                                      uint16_t sequence) {
  auto reference = readUint64(bytes, 0);
  uint8_t length = sliceBytes(bytes, kFileNameLengthOffset, 1)[0];
  uint8_t space = sliceBytes(bytes, kFileNameNamespaceOffset, 1)[0];
  requireValid(space <= 3, "Invalid filename namespace");
  if (space == 2) {
    return {}; // DOS alias of an already listed long name.
  }
  sliceBytes(bytes, kFileNameTextOffset, size_t(length) * 2);
  std::wstring name;
  name.reserve(length);
  for (size_t i = 0; i < length; ++i) {
    wchar_t c = wchar_t(readUint16(bytes, kFileNameTextOffset + i * 2));
    requireValid(c != 0 && c != L'/' && c != L'\\',
                 "Invalid filename component");
    name.push_back(c);
  }
  // Root has a '.' entry, which is not a child.
  if (name.empty() || name == L"." || name == L"..") {
    return {};
  }
  return FileName{recordNumber(reference), owner, sequenceNumber(reference),
                  sequence, std::move(name)};
}

void parseRecord(std::span<uint8_t> bytes, uint32_t id, RecordInfo &slot,
                 ParsedRecords &out) {
  size_t start = out.names.size();
  try {
    applyFixup(bytes);
    uint16_t flags = readUint16(bytes, kRecordFlagsOffset);
    if (!(flags & kRecordInUse)) {
      return;
    }
    uint64_t base = readUint64(bytes, kRecordBaseReferenceOffset);
    bool extension = (base & kReferenceMask) != 0;
    uint32_t owner = extension ? recordNumber(base) : id;
    uint16_t sequence = extension
                            ? sequenceNumber(base)
                            : readUint16(bytes, kRecordSequenceNumberOffset);
    RecordInfo info;
    info.live = true;
    info.directory = (flags & kRecordDirectory) != 0;
    info.sequence = sequence;
    std::optional<uint64_t> apparent;
    uint32_t name_tag = 0;
    for (auto a : parseAttributes(bytes)) {
      switch (a.kind()) {
      case kAttributeStandardInformation:
        info.flags =
            readUint32(a.value(), kStandardInformationAttributesOffset);
        break;
      case kAttributeFileName: {
        auto v = a.value();
        if (readUint32(v, kFileNameAttributesOffset) &
            kFileAttributeReparsePoint) {
          name_tag = readUint32(v, kFileNameReparseTagOffset);
        }
        if (auto name = parseFileName(v, owner, sequence)) {
          out.names.push_back(std::move(*name));
        }
        break;
      }
      case kAttributeData: {
        auto [length, allocated] = a.sizes();
        info.allocated = checkedAdd(info.allocated, allocated);
        if (a.isUnnamed() && a.isFirstExtent()) {
          apparent = length;
        }
        break;
      }
      case kAttributeReparsePoint:
        if (!a.isNonResident()) {
          info.tag = readUint32(a.value(), 0);
        }
        break;
      default:
        break;
      }
    }
    if (!info.tag) {
      info.tag = name_tag;
    }
    info.apparent = apparent.value_or(0);
    if (extension) {
      out.extra.push_back(
          {owner, sequence, apparent, info.allocated, info.tag});
    } else {
      slot = info;
    }
  } catch (const std::runtime_error &) {
    out.names.resize(start);
    ++out.bad;
  }
}

void mergeExtra(std::vector<RecordInfo> &infos, const ParsedRecords &parsed) {
  for (auto &e : parsed.extra) {
    if (e.owner >= infos.size()) {
      continue;
    }
    auto &info = infos[e.owner];
    if (!info.live || info.sequence != e.sequence) {
      continue;
    }
    info.allocated = checkedAdd(info.allocated, e.allocated);
    if (e.apparent) {
      info.apparent = *e.apparent;
    }
    if (!info.tag) {
      info.tag = e.tag;
    }
  }
}

bool isValidName(const FileName &n, const std::vector<RecordInfo> &infos) {
  return n.child >= 16 && n.child != n.parent && n.child < infos.size() &&
         n.parent < infos.size() && infos[n.child].live &&
         infos[n.child].sequence == n.child_sequence && infos[n.parent].live &&
         infos[n.parent].directory &&
         infos[n.parent].sequence == n.parent_sequence;
}

std::vector<ReadRequest> planReads(const std::vector<Run> &runs, Bytes bitmap,
                                   const Geometry &geometry, uint64_t count) {
  requireValid(count <= UINT32_MAX &&
                   count <= checkedMultiply(bitmap.size(), 8),
               "Invalid MFT bitmap length");
  uint64_t align = std::max(uint64_t(1), geometry.cluster / geometry.record);
  uint64_t most = (16u << 20) / geometry.record,
           gap = (1u << 20) / geometry.record;
  auto used = [&](uint64_t n) {
    return (bitmap[size_t(n / 8)] & (1 << (n % 8))) != 0;
  };

  std::vector<ReadRequest> out;
  uint64_t first = 0;
  for (auto r : runs) {
    requireValid(r.length % geometry.record == 0, "Unaligned MFT run");
    uint64_t records = r.length / geometry.record;
    uint64_t limit = std::min(records, count - std::min(first, count));
    uint64_t offset = 0;
    while (offset < limit) {
      if (!used(first + offset)) {
        ++offset;
        continue;
      }
      uint64_t start = offset - offset % align, end = offset + 1, probe = end;
      while (probe < limit && probe - start < most && probe - end < gap) {
        if (used(first + probe)) {
          end = probe + 1;
        }
        ++probe;
      }
      end = std::min(records, ((end + align - 1) / align) * align);
      out.push_back(
          {checkedAdd(r.offset, checkedMultiply(start, geometry.record)),
           checkedMultiply(end - start, geometry.record), first + start});
      offset = end;
    }
    first = checkedAdd(first, records);
  }
  requireValid(first >= count, "MFT stream shorter than its size");
  return out;
}

uint32_t Attribute::kind() const { return readUint32(bytes, 0); }

bool Attribute::isNonResident() const { return bytes[8] != 0; }

bool Attribute::isUnnamed() const { return bytes[9] == 0; }

bool Attribute::isFirstExtent() const {
  return !isNonResident() ||
         readUint64(bytes, kNonResidentStartingVcnOffset) == 0;
}

Bytes Attribute::value() const {
  requireValid(!isNonResident(), "Expected resident attribute");
  auto offset = readUint16(bytes, kResidentValueOffset);
  requireValid(offset >= kResidentHeaderSize, "Invalid resident value offset");
  return sliceBytes(bytes, offset,
                    readUint32(bytes, kResidentValueLengthOffset));
}

std::pair<uint64_t, uint64_t> Attribute::sizes() const {
  if (!isNonResident()) {
    return {value().size(), 0};
  }
  if (!isFirstExtent()) {
    return {0, 0};
  }
  return {readUint64(bytes, kNonResidentDataSizeOffset),
          readUint64(bytes, (readUint16(bytes, kAttributeFlagsOffset) &
                             (kAttributeSparse | kAttributeCompressed))
                                ? kNonResidentCompressedSizeOffset
                                : kNonResidentAllocatedSizeOffset)};
}

} // namespace ntfs

XAMP_BASE_NAMESPACE_END
