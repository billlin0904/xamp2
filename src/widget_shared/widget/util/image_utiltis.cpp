#include <widget/util/image_utiltis.h>

#include <QPainter>
#include <QGraphicsPixmapItem>
#include <QGraphicsBlurEffect>
#include <QSaveFile>
#include <QGraphicsScene>
#include <QImageReader>

#include <widget/widget_shared.h>
#include <widget/imagecache.h>

#include <base/executor.h>

#include <base/object_pool.h>
#include <base/stopwatch.h>
#include <base/fs.h>

#include <thememanager.h>

namespace image_utils {

namespace {
	inline constexpr uint16_t kStackblurMul[255] = {
			512,512,456,512,328,456,335,512,405,328,271,456,388,335,292,512,
			454,405,364,328,298,271,496,456,420,388,360,335,312,292,273,512,
			482,454,428,405,383,364,345,328,312,298,284,271,259,496,475,456,
			437,420,404,388,374,360,347,335,323,312,302,292,282,273,265,512,
			497,482,468,454,441,428,417,405,394,383,373,364,354,345,337,328,
			320,312,305,298,291,284,278,271,265,259,507,496,485,475,465,456,
			446,437,428,420,412,404,396,388,381,374,367,360,354,347,341,335,
			329,323,318,312,307,302,297,292,287,282,278,273,269,265,261,512,
			505,497,489,482,475,468,461,454,447,441,435,428,422,417,411,405,
			399,394,389,383,378,373,368,364,359,354,350,345,341,337,332,328,
			324,320,316,312,309,305,301,298,294,291,287,284,281,278,274,271,
			268,265,262,259,257,507,501,496,491,485,480,475,470,465,460,456,
			451,446,442,437,433,428,424,420,416,412,408,404,400,396,392,388,
			385,381,377,374,370,367,363,360,357,354,350,347,344,341,338,335,
			332,329,326,323,320,318,315,312,310,307,304,302,299,297,294,292,
			289,287,285,282,280,278,275,273,271,269,267,265,263,261,259
	};

	inline constexpr uint8_t kStackblurShr[255] = {
			9, 11, 12, 13, 13, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 17,
			17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19,
			19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20,
			20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21,
			21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
			21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22,
			22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
			22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23,
			23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
			23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
			23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
			23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
			24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
			24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
			24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
			24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24
	};

	void stackblurJob(uint8_t* src,
		uint32_t w,
		uint32_t h,
		uint32_t radius,
		int32_t cores,
		int32_t core,
		int32_t step,
		uint8_t* stack) {
		uint32_t x, y, xp, yp, i;
		uint32_t sp;
		uint32_t stack_start;
		uint8_t* stack_ptr;

		uint8_t* src_ptr;
		uint8_t* dst_ptr;

		uint32_t sum_r;
		uint32_t sum_g;
		uint32_t sum_b;
		uint32_t sum_a;
		uint32_t sum_in_r;
		uint32_t sum_in_g;
		uint32_t sum_in_b;
		uint32_t sum_in_a;
		uint32_t sum_out_r;
		uint32_t sum_out_g;
		uint32_t sum_out_b;
		uint32_t sum_out_a;

		uint32_t wm = w - 1;
		uint32_t hm = h - 1;
		uint32_t w4 = w * 4;
		uint32_t div = (radius * 2) + 1;
		uint32_t mul_sum = kStackblurMul[radius];
		uint8_t shr_sum = kStackblurShr[radius];

		if (step == 1) {
			int32_t minY = core * h / cores;
			int32_t maxY = (core + 1) * h / cores;

			for (y = minY; y < maxY; y++) {
				sum_r = sum_g = sum_b = sum_a =
					sum_in_r = sum_in_g = sum_in_b = sum_in_a =
					sum_out_r = sum_out_g = sum_out_b = sum_out_a = 0;

				src_ptr = src + w4 * y; // start of line (0,y)

				for (i = 0; i <= radius; i++) {
					stack_ptr = &stack[4 * i];
					stack_ptr[0] = src_ptr[0];
					stack_ptr[1] = src_ptr[1];
					stack_ptr[2] = src_ptr[2];
					stack_ptr[3] = src_ptr[3];
					sum_r += src_ptr[0] * (i + 1);
					sum_g += src_ptr[1] * (i + 1);
					sum_b += src_ptr[2] * (i + 1);
					sum_a += src_ptr[3] * (i + 1);
					sum_out_r += src_ptr[0];
					sum_out_g += src_ptr[1];
					sum_out_b += src_ptr[2];
					sum_out_a += src_ptr[3];
				}

				for (i = 1; i <= radius; i++) {
					if (i <= wm) src_ptr += 4;
					stack_ptr = &stack[4 * (i + radius)];
					stack_ptr[0] = src_ptr[0];
					stack_ptr[1] = src_ptr[1];
					stack_ptr[2] = src_ptr[2];
					stack_ptr[3] = src_ptr[3];
					sum_r += src_ptr[0] * (radius + 1 - i);
					sum_g += src_ptr[1] * (radius + 1 - i);
					sum_b += src_ptr[2] * (radius + 1 - i);
					sum_a += src_ptr[3] * (radius + 1 - i);
					sum_in_r += src_ptr[0];
					sum_in_g += src_ptr[1];
					sum_in_b += src_ptr[2];
					sum_in_a += src_ptr[3];
				}

				sp = radius;
				xp = radius;
				if (xp > wm) xp = wm;
				src_ptr = src + 4 * (xp + y * w); //   img.pix_ptr(xp, y);
				dst_ptr = src + y * w4; // img.pix_ptr(0, y);
				for (x = 0; x < w; x++) {
					dst_ptr[0] = (sum_r * mul_sum) >> shr_sum;
					dst_ptr[1] = (sum_g * mul_sum) >> shr_sum;
					dst_ptr[2] = (sum_b * mul_sum) >> shr_sum;
					dst_ptr[3] = (sum_a * mul_sum) >> shr_sum;
					dst_ptr += 4;

					sum_r -= sum_out_r;
					sum_g -= sum_out_g;
					sum_b -= sum_out_b;
					sum_a -= sum_out_a;

					stack_start = sp + div - radius;
					if (stack_start >= div) stack_start -= div;
					stack_ptr = &stack[4 * stack_start];

					sum_out_r -= stack_ptr[0];
					sum_out_g -= stack_ptr[1];
					sum_out_b -= stack_ptr[2];
					sum_out_a -= stack_ptr[3];

					if (xp < wm) {
						src_ptr += 4;
						++xp;
					}

					stack_ptr[0] = src_ptr[0];
					stack_ptr[1] = src_ptr[1];
					stack_ptr[2] = src_ptr[2];
					stack_ptr[3] = src_ptr[3];

					sum_in_r += src_ptr[0];
					sum_in_g += src_ptr[1];
					sum_in_b += src_ptr[2];
					sum_in_a += src_ptr[3];
					sum_r += sum_in_r;
					sum_g += sum_in_g;
					sum_b += sum_in_b;
					sum_a += sum_in_a;

					++sp;
					if (sp >= div) sp = 0;
					stack_ptr = &stack[sp * 4];

					sum_out_r += stack_ptr[0];
					sum_out_g += stack_ptr[1];
					sum_out_b += stack_ptr[2];
					sum_out_a += stack_ptr[3];
					sum_in_r -= stack_ptr[0];
					sum_in_g -= stack_ptr[1];
					sum_in_b -= stack_ptr[2];
					sum_in_a -= stack_ptr[3];
				}
			}
		}

		// step 2
		if (step == 2) {
			int32_t minX = core * w / cores;
			int32_t maxX = (core + 1) * w / cores;

			for (x = minX; x < maxX; x++) {
				sum_r = sum_g = sum_b = sum_a =
					sum_in_r = sum_in_g = sum_in_b = sum_in_a =
					sum_out_r = sum_out_g = sum_out_b = sum_out_a = 0;

				src_ptr = src + 4 * x; // x,0
				for (i = 0; i <= radius; i++) {
					stack_ptr = &stack[i * 4];
					stack_ptr[0] = src_ptr[0];
					stack_ptr[1] = src_ptr[1];
					stack_ptr[2] = src_ptr[2];
					stack_ptr[3] = src_ptr[3];
					sum_r += src_ptr[0] * (i + 1);
					sum_g += src_ptr[1] * (i + 1);
					sum_b += src_ptr[2] * (i + 1);
					sum_a += src_ptr[3] * (i + 1);
					sum_out_r += src_ptr[0];
					sum_out_g += src_ptr[1];
					sum_out_b += src_ptr[2];
					sum_out_a += src_ptr[3];
				}
				for (i = 1; i <= radius; i++) {
					if (i <= hm) src_ptr += w4; // +stride

					stack_ptr = &stack[4 * (i + radius)];
					stack_ptr[0] = src_ptr[0];
					stack_ptr[1] = src_ptr[1];
					stack_ptr[2] = src_ptr[2];
					stack_ptr[3] = src_ptr[3];
					sum_r += src_ptr[0] * (radius + 1 - i);
					sum_g += src_ptr[1] * (radius + 1 - i);
					sum_b += src_ptr[2] * (radius + 1 - i);
					sum_a += src_ptr[3] * (radius + 1 - i);
					sum_in_r += src_ptr[0];
					sum_in_g += src_ptr[1];
					sum_in_b += src_ptr[2];
					sum_in_a += src_ptr[3];
				}

				sp = radius;
				yp = radius;
				if (yp > hm) yp = hm;
				src_ptr = src + 4 * (x + yp * w); // img.pix_ptr(x, yp);
				dst_ptr = src + 4 * x; 			  // img.pix_ptr(x, 0);
				for (y = 0; y < h; y++) {
					dst_ptr[0] = (sum_r * mul_sum) >> shr_sum;
					dst_ptr[1] = (sum_g * mul_sum) >> shr_sum;
					dst_ptr[2] = (sum_b * mul_sum) >> shr_sum;
					dst_ptr[3] = (sum_a * mul_sum) >> shr_sum;
					dst_ptr += w4;

					sum_r -= sum_out_r;
					sum_g -= sum_out_g;
					sum_b -= sum_out_b;
					sum_a -= sum_out_a;

					stack_start = sp + div - radius;
					if (stack_start >= div) stack_start -= div;
					stack_ptr = &stack[4 * stack_start];

					sum_out_r -= stack_ptr[0];
					sum_out_g -= stack_ptr[1];
					sum_out_b -= stack_ptr[2];
					sum_out_a -= stack_ptr[3];

					if (yp < hm) {
						src_ptr += w4; // stride
						++yp;
					}

					stack_ptr[0] = src_ptr[0];
					stack_ptr[1] = src_ptr[1];
					stack_ptr[2] = src_ptr[2];
					stack_ptr[3] = src_ptr[3];

					sum_in_r += src_ptr[0];
					sum_in_g += src_ptr[1];
					sum_in_b += src_ptr[2];
					sum_in_a += src_ptr[3];
					sum_r += sum_in_r;
					sum_g += sum_in_g;
					sum_b += sum_in_b;
					sum_a += sum_in_a;

					++sp;
					if (sp >= div) sp = 0;
					stack_ptr = &stack[sp * 4];

					sum_out_r += stack_ptr[0];
					sum_out_g += stack_ptr[1];
					sum_out_b += stack_ptr[2];
					sum_out_a += stack_ptr[3];
					sum_in_r -= stack_ptr[0];
					sum_in_g -= stack_ptr[1];
					sum_in_b -= stack_ptr[2];
					sum_in_a -= stack_ptr[3];
				}
			}
		}

	}

	void stackblurJob(QImage& image, uint32_t radius = 10, uint32_t cores = std::thread::hardware_concurrency()) {
		XAMP_EXPECTS(radius > 0 && radius < 254);
		XAMP_EXPECTS(cores > 0);

		auto div = (radius * 2) + 1;
		auto stack = Vector<uint8_t>(div * 4 * cores);

		const auto width = image.width();
		const auto height = image.height();
		auto* src = image.bits();		

		if (cores == 1) {
			stackblurJob(src, width, height, radius, 1, 0, 1, stack.data());
			stackblurJob(src, width, height, radius, 1, 0, 2, stack.data());
			return;
		}
		
		auto blur_job = [&](auto step) {
			Vector<Task<>> tasks;
			tasks.reserve(cores);

			for (auto i = 0; i < cores; i++) {
				auto buffer = stack.data() + div * 4 * i;
				tasks.push_back(Executor::Spawn(GetBackgroundThreadPool(),
					[=](const StopToken& stop_token) {
						stackblurJob(src, width, height, radius, cores, i, step, buffer);
					}));
			}

			for (auto& task : tasks) {
				task.get();
			}
			};

		blur_job(1);
		blur_job(2);
	}
}	

bool optimizePng(const QByteArray& buffer, const QString& dest_file_path) {
	QSaveFile file(dest_file_path);	
	file.open(QIODevice::WriteOnly);
	file.write(buffer);
	return file.commit();	
}

bool moveFile(const QString& src_file_path, const QString& dest_file_path) {
	try {
		Fs::rename(src_file_path.toStdWString(), dest_file_path.toStdWString());
	} catch (...) {
		return false;
	}
    return true;
}

QPixmap resizeImage(const QPixmap& source, const QSize& size, bool is_aspect_ratio) {
	const auto scaled_size = source.size() * 2;
	const auto mode = is_aspect_ratio ? Qt::KeepAspectRatio
		                  : Qt::IgnoreAspectRatio;

	const auto temp = source.scaled(scaled_size, mode);
	auto scaled_image = temp.scaled(size, mode, Qt::SmoothTransformation);
	return scaled_image;
}

QByteArray image2ByteArray(const QPixmap& source) {
	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	source.save(&buffer, ImageCache::kImageFileFormat);
	return bytes;
}

Vector<uint8_t> image2Buffer(const QPixmap& source) {
	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	source.save(&buffer, ImageCache::kImageFileFormat);
	return { bytes.constData(), bytes.constData() + bytes.size() };
}

QPixmap convertToImageFormat(const QPixmap& source, int32_t quality) {
	QByteArray bytes;
	QBuffer buffer(&bytes);

	auto image = source.toImage();

	QImage temp(image.size(), QImage::Format_ARGB32);
	temp.fill(QColor(Qt::white).rgb());

	QPainter painter(&temp);
	painter.drawImage(0, 0, image);

	temp.save(&buffer, "JPG", quality);

	QPixmap pixmap;
	pixmap.loadFromData(bytes);

	return pixmap;
}

QPixmap roundImage(const QPixmap& src, int32_t radius) {
	return roundImage(src, src.size(), radius);
}

QPixmap roundDarkImage(QSize size, int32_t alpha, int32_t radius) {
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

QPixmap roundImage(const QPixmap& src, QSize size, int32_t radius) {
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
		painter.drawPixmap(rect, resizeImage(pixmap, size, true));
	} else {
		painter.drawPixmap(rect, pixmap);
	}
	return result;
}

QPixmap gaussianBlur(const QPixmap& source, uint32_t radius) {
	auto image = source.toImage();
	stackblurJob(image, radius);
	return QPixmap::fromImage(image);
}

QImage blurImage(const QPixmap& source, QSize size) {
	const QSize scaled_size(size.width() + kImageBlurRadius, size.height() + kImageBlurRadius);
	auto resize_pixmap = resizeImage(source, scaled_size);
	auto img = resize_pixmap.toImage();
	stackblurJob(img, kImageBlurRadius);
	return img;
}

QPixmap readFileImage(const QString& file_path, QSize size, QImage::Format format) {
	QImage image(size, format);
	QImageReader reader(file_path);
	if (reader.read(&image)) {
		return QPixmap::fromImage(image);
	}
	return {};
}

int sampleImageBlur(const QImage& image, int blur_alpha) {
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
