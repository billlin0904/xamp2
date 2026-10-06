// Integration checks for the NTFS metadata backend and its path fallback.
// Run against matching xamp_base/xamp_metadata DLLs. MFT access is reported
// as unavailable (not passed) when the process lacks volume-read permission.
#include <metadata/imetadatascanreader.h>
#include <metadata/taglibmetareader.h>
#include <base/logger.h>
#include <base/ntfsfilestream.h>
#include <base/threadpoolbuilder.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <system_error>

using namespace xamp::base;
using namespace xamp::metadata;

namespace {
	void require(bool value, const char* message) {
		if (!value) {
			throw std::runtime_error(message);
		}
	}

	struct Fixture final {
		Path directory;
		Path audio = directory / L"\u97f3\u6a02.wav";
		Path broken = directory / "missing.wav";
		std::vector<char> bytes;

		explicit Fixture(const Path& root)
			: directory(root / ("xamp-ntfs-test-" +
				std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))) {
			Fs::create_directory(directory);
			bytes.resize(44 + 16000, 0);
			auto put = [&](size_t offset, uint32_t value, size_t count) {
				for (size_t i = 0; i < count; ++i) {
					bytes[offset + i] = static_cast<char>(value >> (i * 8));
				}
			};
			std::copy_n("RIFF", 4, bytes.data());
			put(4, static_cast<uint32_t>(bytes.size() - 8), 4);
			std::copy_n("WAVEfmt ", 8, bytes.data() + 8);
			put(16, 16, 4); put(20, 1, 2); put(22, 1, 2);
			put(24, 8000, 4); put(28, 16000, 4); put(32, 2, 2); put(34, 16, 2);
			std::copy_n("data", 4, bytes.data() + 36);
			put(40, 16000, 4);
			for (size_t i = 44; i < bytes.size(); ++i) {
				bytes[i] = static_cast<char>(i * 17 + i / 251);
			}
			std::ofstream output(audio, std::ios::binary);
			output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
			require(bool(output), "Failed to write WAV fixture");
		}

		~Fixture() {
			std::error_code error;
			Fs::remove(audio, error);
			Fs::remove(broken, error);
			Fs::remove(directory, error);
		}
	};

	void checkStrictNtfs(const Fixture& fixture) {
		try {
			NtfsFileStream stream(fixture.audio);
			require(stream.size() == fixture.bytes.size(), "NTFS length mismatch");
			std::vector<char> bytes(fixture.bytes.size());
			require(stream.read(bytes.data(), bytes.size()) == bytes.size() && bytes == fixture.bytes,
				"NTFS data differs from original file");
			require(stream.read(bytes.data(), 1) == 0, "NTFS EOF mismatch");
			stream.seek(513, SEEK_SET);
			require(stream.tell() == 513 && stream.read(bytes.data(), 101) == 101 &&
				std::equal(bytes.begin(), bytes.begin() + 101, fixture.bytes.begin() + 513),
				"NTFS random read mismatch");
			stream.seek(-10, SEEK_END);
			require(stream.read(bytes.data(), 101) == 10, "NTFS short read mismatch");
			TaglibMetadataReader reader;
			reader.openNtfs(fixture.audio);
			auto track = reader.extract();
			require(track && track->sample_rate == 8000 && track->file_size == fixture.bytes.size(),
				"Strict NTFS metadata extraction failed");
			std::cout << "PASS: strict NTFS MFT, raw data-run reads and TagLib metadata\n";
		} catch (const std::system_error& error) {
			if (error.code().value() == 50 &&
				std::string(error.what()).find("actual file system=") != std::string::npos) {
				std::cout << "PASS: non-NTFS volume rejected before raw access: " << error.what() << '\n';
				return;
			}
			// Win32 access denied / privilege not held. Other errors must fail.
			if (error.code().value() != 5 && error.code().value() != 1314) {
				throw;
			}
			std::cout << "UNAVAILABLE: strict NTFS MFT access: " << error.what() << '\n';
		}
	}

	void checkReader(const std::shared_ptr<IMetadataScanReader>& reader, const Fixture& fixture) {
		MetadataDirectoryFiles files;
		files[fixture.directory] = {fixture.audio, fixture.broken};
		std::atomic_size_t completed = 0;
		size_t directories = 0;
		size_t tracks = 0;
		MetadataReadCallbacks callbacks;
		callbacks.on_file_completed = [&] { ++completed; };
		callbacks.on_directory_read = [&](const Path& directory, size_t file_count, auto result) {
			++directories;
			require(directory == fixture.directory && file_count == 2, "Wrong directory callback");
			for (const auto& track : result) {
				++tracks;
				require(track.file_path == fixture.audio && track.sample_rate == 8000 &&
					track.file_size == fixture.bytes.size(), "Unexpected metadata");
			}
		};
		reader->read(files, {}, callbacks);
		require(completed == 2 && directories == 1 && tracks == 1,
			"Retry duplicated progress or lost a track");
		std::stop_source stop;
		stop.request_stop();
		reader->read(files, stop.get_token(), callbacks);
		reader->read({}, {}, callbacks);
		require(completed == 2 && directories == 1, "Cancelled/empty scan invoked callbacks");
	}
}

int main(int argc, char** argv) {
	try {
		XampLoggerFactory.addDebugOutput().startup();
		// Optional fixture root allows testing actual ReFS/exFAT fallback.
		Fixture fixture(argc > 1 ? Path(argv[1]) : Fs::temp_directory_path());
		checkStrictNtfs(fixture);
		auto pool = ThreadPoolBuilder::makeThreadPool("NtfsTest", 2, 1);
		checkReader(makeNtfsMetadataScanReader(pool), fixture);
		checkReader(makePathMetadataScanReader(pool), fixture);
		checkReader(makeMetadataScanReader(pool), fixture);
		std::cout << "PASS: NTFS/default/path backend metadata, fallback, progress, cancellation and empty input\n";
		return 0;
	} catch (const std::exception& error) {
		std::cerr << "FAIL: " << error.what() << '\n';
		return 1;
	}
}
