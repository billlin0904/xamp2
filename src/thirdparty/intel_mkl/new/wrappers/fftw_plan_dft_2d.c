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
 * fftw_plan_dft_2d - FFTW3 wrapper to Intel(R) oneAPI Math Kernel Library (Intel(R) oneMKL).
 *
 ******************************************************************************
 */

#include "fftw3_mkl.h"
#ifdef DFT_ENABLE_OFFLOAD
#include "oneapi/mkl/export.hpp"
#include "fftw3_omp_offload_common.h"

DLL_EXPORT fftw_plan
fftw_plan_dft_2d_omp_offload(int nx, int ny, fftw_complex *in, fftw_complex *out,
                             int sign, unsigned flags, void* interopObj)
{
    int n[2] = { nx, ny };
    return fftw_plan_dft_omp_offload_impl(2, n, in, out, sign, flags, interopObj);
}
#else
fftw_plan
fftw_plan_dft_2d(int nx, int ny, fftw_complex *in, fftw_complex *out, int sign,
                 unsigned flags)
{
    int n[2] = { nx, ny };
    return fftw_plan_dft(2, n, in, out, sign, flags);
}
#endif
