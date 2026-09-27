#include "pcm_dsd_simd.h"
#include <immintrin.h>

/* Separate translation unit: only called after AVX2 + OS YMM-state detection.
   No FMA; retain the scalar multiply/add/subtract rounding order. */
void pcm_dsd_multiply_avx2(complex64* values, const complex64* filter, size_t count) {
    size_t i=0;
    for(;count-i>=2;i+=2) {
        const __m256d a=_mm256_loadu_pd((const double*)(values+i));
        const __m256d b=_mm256_loadu_pd((const double*)(filter+i));
        const __m256d re=_mm256_movedup_pd(a);
        const __m256d im=_mm256_permute_pd(a,15);
        const __m256d swapped=_mm256_permute_pd(b,5);
        _mm256_storeu_pd((double*)(values+i),
            _mm256_addsub_pd(_mm256_mul_pd(re,b),_mm256_mul_pd(im,swapped)));
    }
    for(;i<count;i++) {
        const complex64 a=values[i],b=filter[i];
        values[i].re=a.re*b.re-a.im*b.im;
        values[i].im=a.re*b.im+a.im*b.re;
    }
    _mm256_zeroupper();
}
