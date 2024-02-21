#include "supereq.h"

std::unique_ptr<supereq_base> make_supereq() {
	return std::make_unique<supereq<float>>();
}