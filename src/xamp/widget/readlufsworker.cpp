#include "readlufsworker.h"

#include <player/loudness_scanner.h>

#include <base/buffer.h>
#include <base/audioformat.h>
#include <base/dsdsampleformat.h>

#include <stream/filestream.h>
#include <stream/dsdstream.h>
#include <stream/stream_util.h>

#include <widget/playlistentity.h>
#include <widget/readlufsworker.h>

using namespace xamp::base;
using namespace xamp::stream;
using namespace xamp::player;

inline constexpr uint32_t kReadSampleSize = 8192 * 4;

ReadLufsWorker::ReadLufsWorker() = default;

void ReadLufsWorker::addEntity(PlayListEntity const& entity) {
	const auto is_dsd_file = TestDsdFileFormatStd(entity.file_path.toStdWString());
	auto file_stream = MakeStream(entity.file_ext.toStdWString());

	// DSD�榡�L�kŪ�����T��, �G�ݭn�ഫ��PCM�榡.
	if (auto* stream = dynamic_cast<DsdStream*>(file_stream.get())) {
		if (is_dsd_file) {
			stream->SetDSDMode(DsdModes::DSD_MODE_DSD2PCM);
		}
    }

    try {
        file_stream->OpenFile(entity.file_path.toStdWString());
    } catch (...) {
        return;
    }

	const auto source_format = file_stream->GetFormat();
	const auto input_format = AudioFormat::ToFloatFormat(source_format);

	LoudnessScanner scanner(input_format.GetSampleRate());

	auto isamples = MakeBuffer<float>(1024 + kReadSampleSize * input_format.GetChannels());
	uint32_t num_samples = 0;

	while (true) {
		const auto read_size = file_stream->GetSamples(isamples.get(),
			kReadSampleSize) / input_format.GetChannels();

		if (!read_size) {
			break;
		}
		scanner.Process(isamples.get(), read_size * input_format.GetChannels());
	}

	emit readCompleted(entity.music_id, scanner.GetLoudness(), scanner.GetTruePeek());
}
