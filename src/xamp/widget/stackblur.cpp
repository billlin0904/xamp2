#include <array>
#include <thread>
#include <base/buffer.h>
#include <base/ithreadpool.h>
#include <widget/stackblur.h>

using namespace xamp::base;

static constexpr std::array<uint16_t, 255> kStackblurMul{
	512, 512, 456, 512, 328, 456, 335, 512, 405, 328, 271, 456, 388, 335, 292, 512,
	454, 405, 364, 328, 298, 271, 496, 456, 420, 388, 360, 335, 312, 292, 273, 512,
	482, 454, 428, 405, 383, 364, 345, 328, 312, 298, 284, 271, 259, 496, 475, 456,
	437, 420, 404, 388, 374, 360, 347, 335, 323, 312, 302, 292, 282, 273, 265, 512,
	497, 482, 468, 454, 441, 428, 417, 405, 394, 383, 373, 364, 354, 345, 337, 328,
	320, 312, 305, 298, 291, 284, 278, 271, 265, 259, 507, 496, 485, 475, 465, 456,
	446, 437, 428, 420, 412, 404, 396, 388, 381, 374, 367, 360, 354, 347, 341, 335,
	329, 323, 318, 312, 307, 302, 297, 292, 287, 282, 278, 273, 269, 265, 261, 512,
	505, 497, 489, 482, 475, 468, 461, 454, 447, 441, 435, 428, 422, 417, 411, 405,
	399, 394, 389, 383, 378, 373, 368, 364, 359, 354, 350, 345, 341, 337, 332, 328,
	324, 320, 316, 312, 309, 305, 301, 298, 294, 291, 287, 284, 281, 278, 274, 271,
	268, 265, 262, 259, 257, 507, 501, 496, 491, 485, 480, 475, 470, 465, 460, 456,
	451, 446, 442, 437, 433, 428, 424, 420, 416, 412, 408, 404, 400, 396, 392, 388,
	385, 381, 377, 374, 370, 367, 363, 360, 357, 354, 350, 347, 344, 341, 338, 335,
	332, 329, 326, 323, 320, 318, 315, 312, 310, 307, 304, 302, 299, 297, 294, 292,
	289, 287, 285, 282, 280, 278, 275, 273, 271, 269, 267, 265, 263, 261, 259
};

static constexpr std::array<uint8_t, 255> kStackblurShr{
	9,  11, 12, 13, 13, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 17,
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

static void StackblurJob(uint8_t* src,
	uint32_t w,
	uint32_t h,
	uint32_t radius,
	uint32_t total_threads,
    size_t thread_index,
	int32_t step,
	uint8_t* stack) noexcept {
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
		auto minY = thread_index * h / total_threads;
		auto maxY = (thread_index + 1) * h / total_threads;

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
		auto minX = thread_index * w / total_threads;
		auto maxX = (thread_index + 1) * w / total_threads;

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


Stackblur::Stackblur(IThreadPool& tp, QImage& image, uint32_t radius) {
	blur(tp,
		image.bits(),
		image.width(),
		image.height(),
		radius,
		4);
}

void Stackblur::blur(IThreadPool& tp,
	uint8_t* src,
	uint32_t width,
	uint32_t height,
	uint32_t radius,
	uint32_t thread_branch) {
	if (radius > 254) {
		return;
	}

	if (radius < 2) {
		return;
	}

	auto div = (radius * 2) + 1;
	auto stack = MakeBuffer<uint8_t>(div * 4 * thread_branch);

	if (thread_branch == 1) {
		StackblurJob(src, width, height, radius, 1, 0, 1, stack.get());
		StackblurJob(src, width, height, radius, 1, 0, 2, stack.get());
	}
	else {
		auto blur_job = [div, &stack, src, width, height, radius, thread_branch, &tp](int step) {
            ParallelFor(0, thread_branch, [div, &stack, src, width, height, radius, thread_branch, step](size_t i) {
                auto* buffer = stack.get() + div * 4 * i;
                StackblurJob(src, width, height, radius, thread_branch, i, step, buffer);
				}, tp, thread_branch);
		};
		blur_job(1);
		blur_job(2);
	}
}