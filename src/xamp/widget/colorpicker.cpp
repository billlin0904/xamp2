#include <widget/colorpicker.h>

#define CLIP(X) ( (X) > 255 ? 255 : (X) < 0 ? 0 : X)

// RGB -> YUV
#define RGB2Y(R, G, B) CLIP(( (  66 * (R) + 129 * (G) +  25 * (B) + 128) >> 8) +  16)
#define RGB2U(R, G, B) CLIP(( ( -38 * (R) -  74 * (G) + 112 * (B) + 128) >> 8) + 128)
#define RGB2V(R, G, B) CLIP(( ( 112 * (R) -  94 * (G) -  18 * (B) + 128) >> 8) + 128)

// ÂùÃäÂoªi-°ª´µ¼Ò½k
class BilateralFilter {
public:
	BilateralFilter(int32_t radius, double sigmaD, double sigmaC)
		: radius_(radius)
		, sigmaD_(sigmaD)
		, sigmaC_(sigmaC) {
		auto size = 2 * radius + 1;
		dw_.resize(size);
		for (auto& table : dw_) {
			table.resize(size);
		}
		cw_.resize(256);
		BuildDistanceWeightTable();
		BuildDistanceWeightTable();
	}

	void Apply(QImage& image) const {
		for (auto row = radius_; row < image.width() - radius_; row++) {
			for (auto col = radius_; col < image.height() - radius_; col++) {
				float weightSum[3] = { 0 };
				float pixelSum[3] = { 0 };
				
				auto cur = image.pixelColor(row, col);

				for (auto i = -radius_; i <= radius_; i++) {
					for (auto j = -radius_; j <= radius_; j++) {
						auto neighbor = image.pixelColor(row + i, col + j);

						auto distance_weight = dw_[i + radius_][j + radius_];

						float red_weight = distance_weight * cw_[abs(neighbor.red() - cur.red())];
						pixelSum[0] += neighbor.red() * red_weight;
						weightSum[0] += red_weight;

						float green_weight = distance_weight * cw_[abs(neighbor.green() - cur.green())];
						pixelSum[1] += neighbor.green() * green_weight;
						weightSum[1] += green_weight;

						float blue_weight = distance_weight * cw_[abs(neighbor.blue() - cur.blue())];
						pixelSum[2] += neighbor.blue() * blue_weight;
						weightSum[2] += blue_weight;
					}
				}

				cur.setRed(pixelSum[0] / weightSum[0]);
				cur.setGreen(pixelSum[1] / weightSum[1]);
				cur.setBlue(pixelSum[2] / weightSum[2]);

				image.setPixelColor(row, col, cur);
			}
		}
	}

private:
	void BuildDistanceWeightTable() {
		for (auto i = -radius_; i <= radius_; i++) {
			for (size_t j = -radius_; j <= radius_; j++) {
				dw_[i + radius_][j + radius_]
					= exp(-0.5 * (i * i + j * j) / (sigmaD_ * sigmaD_));
			}
		}
	}

	void BuildColorWeightTable() {
		for (auto i = 0; i < cw_.size(); i++) {
			cw_[i] = exp(-0.5 * (i * i) / (sigmaC_ * sigmaC_));
		}
	}

	int32_t radius_;
	double sigmaD_;
	double sigmaC_;
	std::vector<std::vector<double>> dw_;
	std::vector<double> cw_;
};

ColorPicker::ColorPicker() {
}

void ColorPicker::loadImage(const QPixmap& image) {
	loadImage(image.toImage());
}

QImage ColorPicker::getTestImage(const QImage& image) const {
	static const BilateralFilter filter{ 36, 0.1, 0.1 };
	constexpr QSize image_size{ 36,36 };
	
	auto small_size = image.scaled(image_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	filter.Apply(small_size);
	return small_size;
}

void ColorPicker::loadImage(const QImage& image) {
	/*
	1.Image is scaled to 36x36 px (this reduce the compute time).
	2.It generates a pixel array from the image.
	3.Converts the pixel array to YUV space.
	4.Gather colors as Seth Thompson's code does it.
	5.The color's sets are sorted by count.
	6.The algorithm select the three most dominant colors.
	7.The most dominant is asigned as Background.
	8.The second and third most dominants are tested using the w3c color contrast formula,
	  to check if the colors has enought contrast with the background.
	9.If one of the text colors don't pass the test, then is asigned to white or black,
	  depending of the Y component.
	*/

	auto test_image = getTestImage(image);

	std::vector<uint32_t> buffer;
	buffer.reserve(test_image.width() * test_image.height());

	for (auto x = 0; x < test_image.width(); ++x) {
		for (auto y = 0; y < test_image.height(); ++y) {
			auto pixel = test_image.pixel(x, y);
			auto r = qRed(pixel);
			auto g = qGreen(pixel);
			auto b = qBlue(pixel);
			uint32_t yuv =
				  RGB2Y(r, g, b)
				| RGB2U(r, g, b)
				| RGB2U(r, g, b);
			buffer.push_back(yuv);
		}
	}
}