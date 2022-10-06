/*******************************************************************************
* Copyright 2005-2022 Intel Corporation.
*
* This software and the related documents are Intel copyrighted  materials,  and
* your use of  them is  governed by the  express license  under which  they were
* provided to you (License).  Unless the License provides otherwise, you may not
* use, modify, copy, publish, distribute,  disclose or transmit this software or
* the related documents without Intel's prior written permission.
*
* This software and the related documents  are provided as  is,  with no express
* or implied  warranties,  other  than those  that are  expressly stated  in the
* License.
*******************************************************************************/

/*
 *
 * sfftw_execute_dft - FFTW3 Fortran 77 wrapper to Intel(R) oneAPI Math Kernel Library (Intel(R) oneMKL).
 *
 ******************************************************************************
 */

#include "fftw3_mkl_f77.h"
#ifdef DFT_ENABLE_OFFLOAD
#include "oneapi/mkl/export.hpp"
#include "fftw3_omp_offload_common.h"

void fftwf_execute_dft_omp_offload(const fftwf_plan plan,
                               fftwf_complex *in,
                               fftwf_complex *out,
                               void *interopObj);

#ifdef MKL_ILP64
DLL_EXPORT void
sfftw_execute_dft_omp_offload(PLAN *p,
                              CFI_cdesc_t *in,
                              CFI_cdesc_t *out,
                              void *interopObj)
{
    if (p == NULL) return;
    fftwf_execute_dft_omp_offload(*(fftwf_plan *)p,
                                  (COMPLEX8 *)in->base_addr,
                                  (COMPLEX8 *)out->base_addr,
                                  interopObj);
}

DLL_EXPORT void
sfftw_execute_dft_cpu(PLAN *p,
                      CFI_cdesc_t *in,
                      CFI_cdesc_t *out)
{
    if (p == NULL) return;
    fftwf_execute_dft(*(fftwf_plan *)p,
                      (COMPLEX8 *)in->base_addr,
                      (COMPLEX8 *)out->base_addr);
}
#endif
#else
void
sfftw_execute_dft(PLAN *p, COMPLEX8 *in, COMPLEX8 *out)
{
    if (p == NULL) return;
    fftwf_execute_dft(*(fftwf_plan *)p, in, out);
}
#endif
