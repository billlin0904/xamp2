// Deterministic $DATA tests: no disk/administrator access or external libraries.
#include <base/ntfs.h>

#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace xamp::base::ntfs;

namespace {
void check(bool condition, const char* message) {
	if (!condition) {
		throw std::runtime_error(message);
	}
}

template <typename Function> void rejected(Function&& function) {
	try {
		function();
	} catch (const std::runtime_error&) {
		return;
	}
	throw std::runtime_error("Invalid NTFS input was accepted");
}

void put(Buffer& bytes, size_t offset, uint64_t value, size_t width) {
	for (size_t i = 0; i < width; ++i) {
		bytes.at(offset + i) = static_cast<uint8_t>(value >> (8 * i));
	}
}

Buffer record(const Buffer& attribute) {
	Buffer bytes(1024);
	put(bytes, 0x14, 48, 2);
	put(bytes, 0x18, 48 + attribute.size() + 4, 4);
	std::copy(attribute.begin(), attribute.end(), bytes.begin() + 48);
	put(bytes, 48 + attribute.size(), UINT32_MAX, 4);
	return bytes;
}

Buffer nonresident(Buffer mapping, uint64_t first, uint64_t last, uint64_t size, uint64_t initialized,
				   uint16_t flags = 0) {
	Buffer bytes(((64 + mapping.size() + 7) / 8) * 8);
	put(bytes, 0, 0x80, 4);
	put(bytes, 4, bytes.size(), 4);
	bytes[8] = 1;
	put(bytes, 0x0c, flags, 2);
	put(bytes, 0x10, first, 8);
	put(bytes, 0x18, last, 8);
	put(bytes, 0x20, 64, 2);
	put(bytes, 0x30, size, 8);
	put(bytes, 0x38, initialized, 8);
	std::copy(mapping.begin(), mapping.end(), bytes.begin() + 64);
	return bytes;
}

void testData() {
	Geometry geometry{512, 1024, 1024, 32768};
	Buffer disk(geometry.volume);
	for (size_t i = 0; i < disk.size(); ++i) {
		disk[i] = static_cast<uint8_t>((i / 512) * 29 + i % 251);
	}
	size_t reads = 0;
	VolumeReader read = [&](uint64_t offset, std::span<uint8_t> destination) {
		++reads;
		check(offset <= disk.size() && destination.size() <= disk.size() - offset,
			  "Out-of-bounds physical read");
		std::copy_n(disk.begin() + offset, destination.size(), destination.begin());
	};

	// Relative LCN -2 after a sparse run must still be relative to LCN 6.
	auto attribute = nonresident({0x11, 1, 6, 0x01, 1, 0x11, 1, 0xfe, 0}, 0, 2, 1536, 1280, 0x8000);
	auto data = parseDataStream({record(attribute)}, geometry);
	check(data.runs.size() == 3 && data.runs[1].sparse && data.runs[2].offset == 2048,
		  "Sparse/negative relative run decoding failed");
	Buffer expected(1536);
	std::copy_n(disk.begin() + 3072, 512, expected.begin());
	std::copy_n(disk.begin() + 2048, 256, expected.begin() + 1024);
	Buffer actual(1536, 0xcc);
	check(readData(data, 0, actual, read) == actual.size() && actual == expected && reads == 2,
		  "Raw mapping, sparse zeros or uninitialized tail failed");
	Buffer crossing(600);
	check(readData(data, 500, crossing, read) == 600 &&
			  std::equal(crossing.begin(), crossing.end(), expected.begin() + 500),
		  "Cross-run seek failed");
	Buffer eof(16, 0xcc);
	const auto old_reads = reads;
	check(readData(data, 1528, eof, read) == 8 && reads == old_reads &&
			  std::all_of(eof.begin(), eof.begin() + 8, [](uint8_t value) { return value == 0; }) &&
			  eof[8] == 0xcc,
		  "EOF clamping or uninitialized zero fill failed");
	check(readData(data, UINT64_MAX, eof, read) == 0, "Seek beyond EOF failed");
	check(readData(data, 0, {}, read) == 0, "Empty read failed");
	rejected([&] {
		readData(data, 0, actual, [](uint64_t, std::span<uint8_t>) {
			throw std::runtime_error("Injected volume I/O failure");
		});
	});

	// Resident bytes must be owned, not reference a destroyed MFT record.
	Buffer resident(32);
	put(resident, 0, 0x80, 4);
	put(resident, 4, 32, 4);
	put(resident, 0x10, 3, 4);
	put(resident, 0x14, 24, 2);
	resident[24] = 'a';
	resident[25] = 'b';
	resident[26] = 'c';
	auto small = parseDataStream({record(resident)}, geometry);
	resident.clear();
	Buffer tail(8, 0xcc);
	check(readData(small, 1, tail, {}) == 2 && tail[0] == 'b' && tail[1] == 'c' && tail[2] == 0xcc,
		  "Resident stream lifetime/partial read failed");

	// Extension records may arrive out of order, but VCN coverage must be exact.
	auto base = nonresident({0x11, 2, 6, 0}, 0, 1, 1536, 1536);
	auto extension = nonresident({0x11, 1, 2, 0}, 2, 2, 0, 0);
	auto fragmented = parseDataStream({record(extension), record(base)}, geometry);
	check(fragmented.runs.size() == 2 && fragmented.runs[1].offset == 1024, "Extension ordering failed");
	Buffer joined(32);
	readData(fragmented, 1010, joined, read);
	check(std::equal(joined.begin(), joined.begin() + 14, disk.begin() + 3072 + 1010) &&
			  std::equal(joined.begin() + 14, joined.end(), disk.begin() + 1024),
		  "Fragment boundary read failed");
	rejected([&] { parseDataStream({record(base)}, geometry); });
	rejected([&] { parseDataStream({record(base), record(base)}, geometry); });
	put(extension, 0x10, 3, 8);
	rejected([&] { parseDataStream({record(base), record(extension)}, geometry); });

	for (const auto flags : {0x0001, 0x4000}) {
		auto unsupported = attribute;
		put(unsupported, 0x0c, flags, 2);
		rejected([&] { parseDataStream({record(unsupported)}, geometry); });
	}
	auto invalid = attribute;
	put(invalid, 0x38, 1537, 8);
	rejected([&] { parseDataStream({record(invalid)}, geometry); });
	invalid = nonresident({0x11, 1, 100, 0}, 0, 0, 512, 512);
	rejected([&] { parseDataStream({record(invalid)}, geometry); });
	invalid = nonresident({0x11, 0, 1, 0}, 0, 0, 512, 512);
	rejected([&] { parseDataStream({record(invalid)}, geometry); });
	invalid = nonresident({0x11, 1, 0xff, 0}, 0, 0, 512, 512);
	rejected([&] { parseDataStream({record(invalid)}, geometry); });
	invalid = nonresident({0x88, 1, 1, 1, 1, 1, 1, 1}, 0, 0, 512, 512);
	rejected([&] { parseDataStream({record(invalid)}, geometry); });
	// Existing MFT helpers continue to reject sparse MFT runs.
	rejected([&] { decodeRuns(Attribute{attribute}, 512); });
}
} // namespace

int main() {
	try {
		testData();
		std::cout << "PASS: resident, fragmented, sparse, initialized size, EOF, extension and malformed "
					 "$DATA tests\n";
		return 0;
	} catch (const std::exception& error) {
		std::cerr << "FAIL: " << error.what() << '\n';
		return 1;
	}
}
