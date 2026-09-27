#ifndef PCM_DSD_SIMD_H
#define PCM_DSD_SIMD_H
#include <stddef.h>
typedef struct { double re, im; } complex64;
#ifdef PCM_DSD_USE_SIMD
void pcm_dsd_multiply_avx2(complex64* values, const complex64* filter, size_t count);
#endif
#endif
