#include <widget/image_utiltis.h>

#include <QPainter>
#include <QBuffer>
#include <QGraphicsPixmapItem>
#include <QGraphicsBlurEffect>

#include <widget/widget_shared.h>
#include <widget/pixmapcache.h>

#include <base/stopwatch.h>
#include <base/fs.h>
#include <base/logger_impl.h>

#include "thememanager.h"

//#define USE_ZOPFLIPNG
#define USE_PNGCRUSH

#ifdef USE_ZOPFLIPNG
#include <zopflipng_lib.h>
#include <lodepng/lodepng.h>
#endif

#ifdef USE_PNGCRUSH
#include <libimagequant.h>
#include <lodepng/lodepng.h>
#endif

Q_WIDGETS_EXPORT void qt_blurImage(QPainter* p, QImage& blurImage, qreal radius, bool quality,
	bool alphaOnly, int transposed = 0);

namespace image_utils {

#ifdef USE_PNGCRUSH

namespace pngcrush {
	template <typename T>
	struct LiqResourceDeleter;

	template <typename T>
	struct LodePNGResourceDeleter;

	template <typename T>
	using LiqPtr = std::unique_ptr<T, LiqResourceDeleter<T>>;

	template <typename T>
	using LodePNGPtr = std::unique_ptr<T, LodePNGResourceDeleter<T>>;

	template <>
	struct LiqResourceDeleter<liq_result> {
		void operator()(liq_result* p) const {
			liq_result_destroy(p);
		}
	};

	template <>
	struct LiqResourceDeleter<liq_attr> {
		void operator()(liq_attr* p) const {
			liq_attr_destroy(p);
		}
	};

	template <>
	struct LiqResourceDeleter<liq_image> {
		void operator()(liq_image* p) const {
			liq_image_destroy(p);
		}
	};

	template <>
	struct LodePNGResourceDeleter<LodePNGState> {
		void operator()(LodePNGState* p) const {
			lodepng_state_cleanup(p);
		}
	};

	void OptimizePng(const std::vector<uint8_t>& original_png, std::vector<uint8_t>& result_png) {
		unsigned int width, height;
		unsigned char* raw_rgba_pixels;

		if (lodepng_decode32(&raw_rgba_pixels, &width, &height,
			original_png.data(), original_png.size())) {
			return;
		}

		LiqPtr<liq_attr> handle(liq_attr_create());
		LiqPtr<liq_image> input_image(liq_image_create_rgba(
			handle.get(),
			raw_rgba_pixels, width, height, 0));

		liq_set_quality(handle.get(), 50, 60);

		LiqPtr<liq_result> result(liq_quantize_image(handle.get(), input_image.get()));
		if (!result) {
			return;
		}

		const size_t pixels_size = width * height;

		std::vector<unsigned char> raw_8bit_pixels(pixels_size);
		liq_set_dithering_level(result.get(), 1.0);

		liq_write_remapped_image(result.get(), input_image.get(), raw_8bit_pixels.data(), pixels_size);
		const liq_palette* palette = liq_get_palette(result.get());

		LodePNGState state;
		lodepng_state_init(&state);
		state.info_raw.colortype = LCT_PALETTE;
		state.info_raw.bitdepth = 8;
		state.info_png.color.colortype = LCT_PALETTE;
		state.info_png.color.bitdepth = 8;

		LodePNGPtr<LodePNGState> state_raii(&state);

		for (auto i = 0; i < palette->count; i++) {
			lodepng_palette_add(&state.info_png.color,
				palette->entries[i].r,
				palette->entries[i].g,
				palette->entries[i].b,
				palette->entries[i].a);
			lodepng_palette_add(&state.info_raw,
				palette->entries[i].r,
				palette->entries[i].g,
				palette->entries[i].b,
				palette->entries[i].a);
		}

		unsigned char* output_file_data = nullptr;
		size_t output_file_size = 0;
		const auto out_status =
			lodepng_encode(&output_file_data,
				&output_file_size,
				raw_8bit_pixels.data(),
				width,
				height,
				&state);
		if (out_status) {
			return;
		}

		result_png.resize(output_file_size);
		MemoryCopy(result_png.data(), output_file_data, output_file_size);
	}
}
#endif

#ifdef USE_ZOPFLIPNG
namespace zopflipng {	
	void OptimizePng(const std::vector<uint8_t>& original_png, std::vector<uint8_t> &result_png) {
		ZopfliPNGOptions png_options;
		png_options.use_zopfli = true;
		png_options.lossy_8bit = true;
		png_options.lossy_transparent = true;
		png_options.num_iterations = 20;
		png_options.num_iterations_large = 7;
		png_options.auto_filter_strategy = false;
		png_options.filter_strategies.push_back(kStrategyEntropy);

		if (::ZopfliPNGOptimize(original_png, png_options, png_options.verbose, &result_png)) {
			throw PlatformSpecException();
		}
	}	
}
#endif

template <typename OptimizeFunc>
static void OptimizePng(const QString& dest_file_path, const std::vector<uint8_t> &original_png, std::vector<uint8_t> &result_png, OptimizeFunc func) {
	Stopwatch sw;

	func(original_png, result_png);

	XAMP_LOG_DEBUG("optimize PNG {} => {} ({}% {} secs)",
	               String::FormatBytes(original_png.size()),
	               String::FormatBytes(result_png.size()),
	               static_cast<uint32_t>((static_cast<double>(result_png.size()) / static_cast<double>(original_png.size())) * 100),
	               sw.ElapsedSeconds());

	QFile file(dest_file_path);
	if (!file.open(QIODevice::WriteOnly)) {
		throw PlatformSpecException();
	}

#ifdef XAMP_OS_WIN
	SetFileLowIoPriority(file.handle());
#endif
	file.write(QByteArray(reinterpret_cast<const char*>(result_png.data()), result_png.size()));
}

static void OptimizePng(const QString& dest_file_path, const std::vector<uint8_t>& original_png, std::vector<uint8_t>& result_png) {
#if defined(USE_ZOPFLIPNG)
	OptimizePng(dest_file_path, original_png, result_png, zopflipng::OptimizePng);
#else
	OptimizePng(dest_file_path, original_png, result_png, pngcrush::OptimizePng);
#endif
}

bool OptimizePng(const QByteArray& buffer, const QString& dest_file_path) {
#if defined(USE_ZOPFLIPNG) || defined(USE_PNGCRUSH)
	const std::vector<uint8_t> original_png(buffer.begin(), buffer.end());
	std::vector<uint8_t> result_png;
	OptimizePng(dest_file_path, original_png, result_png);
	return true;
#else
	const auto image = QImage::fromData(buffer);
	return image.save(dest_file_path, PixmapCache::kImageFileFormat, 100);
#endif
}

bool OptimizePng(const QString& src_file_path, const QString& dest_file_path) {
#if defined(USE_ZOPFLIPNG) || defined(USE_PNGCRUSH)
	if (!QFile(src_file_path).exists()) {
		return false;
	}

	auto exception_handler = [](const auto& ex) {
		XAMP_LOG_DEBUG(ex.GetErrorMessage());
	};

	ExceptedFile excepted(dest_file_path.toStdWString());
	return excepted.Try([src_file_path](const auto & dest_file_path) {
		std::vector<uint8_t> original_png;
		std::vector<uint8_t> result_png;
		if (lodepng::load_file(original_png, src_file_path.toLocal8Bit().data())) {
			throw PlatformSpecException();
		}
		OptimizePng(QString::fromStdWString(dest_file_path.wstring()), original_png, result_png);
	}, exception_handler);
#else
	Fs::rename(src_file_path.toStdWString(), dest_file_path.toStdWString());
    return true;
#endif
}

QPixmap ResizeImage(const QPixmap& source, const QSize& size, bool is_aspect_ratio) {
	const auto scaled_size = source.size() * 2;
	const auto mode = is_aspect_ratio ? Qt::KeepAspectRatioByExpanding
		                  : Qt::IgnoreAspectRatio;

	return source.scaled(scaled_size, mode)
		.scaled(size, mode, Qt::SmoothTransformation);
}

QByteArray Convert2ByteArray(const QPixmap& source) {
	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	source.save(&buffer, PixmapCache::kImageFileFormat);
	return bytes;
}

std::vector<uint8_t> Convert2Vector(const QPixmap& source) {
	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	source.save(&buffer, PixmapCache::kImageFileFormat);
	return { bytes.constData(), bytes.constData() + bytes.size() };
}

QPixmap RoundImage(const QPixmap& src, int32_t radius) {
	return RoundImage(src, src.size(), radius);
}

QPixmap RoundDarkImage(QSize size, int32_t alpha, int32_t radius) {
	QColor color = Qt::black;
	color.setAlpha(alpha);
	const QRect darker_rect(0, 0, size.width(), size.height());

	QPixmap result(size);
	result.fill(Qt::transparent);

	QPainter painter(&result);
	painter.setRenderHints(QPainter::Antialiasing, true);
	painter.setRenderHints(QPainter::SmoothPixmapTransform, true);
	painter.setRenderHints(QPainter::TextAntialiasing, true);

	QPainterPath painter_path;
	painter_path.addRoundedRect(darker_rect, radius, radius);
	painter.setClipPath(painter_path);
	painter.fillPath(painter_path, QBrush(color));
	return result;
}

QPixmap RoundImage(const QPixmap& src, QSize size, int32_t radius) {
	QPixmap result(size);
	const QPixmap pixmap(src);

	result.fill(Qt::transparent);

	QPainter painter(&result);
	QPainterPath painter_path;
	const QRect rect(0, 0, size.width(), size.height());

	painter_path.addRoundedRect(rect, radius, radius);
	painter.setRenderHints(QPainter::Antialiasing, true);
	painter.setRenderHints(QPainter::SmoothPixmapTransform, true);
	painter.setRenderHints(QPainter::TextAntialiasing, true);
	painter.setClipPath(painter_path);
	painter.setBrush(QBrush(QColor(249, 249, 249)));
	if (src.size() != size) {
		painter.drawPixmap(rect, ResizeImage(pixmap, size, true));
	} else {
		painter.drawPixmap(rect, pixmap);
	}
	return result;
}

QImage BlurImage(const QPixmap& source, QSize size) {
	const int radius = qMax(20, qMin(size.width(), size.height()) / 5);

	const QSize scaled_size(size.width() + radius, size.height() + radius);
	auto pixmap = ResizeImage(source, scaled_size);
	auto img = pixmap.toImage();

	QPainter painter(&pixmap);
	qt_blurImage(&painter, img, radius, true, false);

	// note: 去除模糊後的黑邊.
	auto clip_size = qMin(source.size().width(), source.size().height());
	clip_size = qMin(clip_size / 2, radius);
	auto clip = pixmap.copy(clip_size,
		clip_size, 
		pixmap.width() - clip_size * 2,
		pixmap.height() - clip_size * 2);

	return pixmap.copy(clip_size,
		clip_size, pixmap.width() - clip_size * 2,
		pixmap.height() - clip_size * 2).toImage();
}

int SampleImageBlur(const QImage& image, int blur_alpha) {
	const auto w = image.width();
	const auto h = image.height();

	qint64 rgb_sum = 0;
	constexpr int m = 16;

	for (int y = 0; y < m; y++) {
		for (int x = 0; x < m; x++) {
			QColor c = image.pixelColor(w * x / m, h * x / m);
			rgb_sum += c.red() + c.green() + c.blue();
		}
	}

	const auto addin = static_cast<int>(rgb_sum * blur_alpha / (255 * 3 * m * m));
	return qMin(255, blur_alpha + addin);
}

}
