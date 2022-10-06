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
 * dfftw_plan_dft_2d - FFTW3 Fortran 77 wrapper to Intel(R) oneAPI Math Kernel Library (Intel(R) oneMKL).
 *
 ******************************************************************************
 */

#include "fftw3_mkl_f77.h"
#ifdef DFT_ENABLE_OFFLOAD
#include "oneapi/mkl/export.hpp"
#include "fftw3_omp_offload_common.h"

DLL_EXPORT void
dfftw_plan_dft_2d_omp_offload(PLAN *p, INTEGER *nx, INTEGER *ny, CFI_cdesc_t *in,
                CFI_cdesc_t *out, INTEGER *sign, INTEGER *flags, void *interopObj)
{
    INTEGER two = 2;
    INTEGER n[2] = { *nx, *ny };
    dfftw_plan_dft_omp_offload_impl(p, &two, n, (COMPLEX16 *)in->base_addr,
                             (COMPLEX16 *)out->base_addr, sign, flags, interopObj);
}

DLL_EXPORT void
dfftw_plan_dft_2d_cpu(PLAN *p, INTEGER *nx, INTEGER *ny, CFI_cdesc_t *in,
                      CFI_cdesc_t *out, INTEGER *sign, INTEGER *flags)
{
    dfftw_plan_dft_2d(p, nx, ny, (COMPLEX16 *)in->base_addr,
                      (COMPLEX16 *)out->base_addr, sign, flags);
}
#else
void
dfftw_plan_dft_2d(PLAN *p, INTEGER *nx, INTEGER *ny, COMPLEX16 *in,
                  COMPLEX16 *out, INTEGER *sign, INTEGER *flags)
{
    INTEGER two = 2;
    INTEGER n[2] = { *nx, *ny };

    dfftw_plan_dft(p, &two, n, in, out, sign, flags);
}
#endif
