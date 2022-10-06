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
 * sfftw_execute - FFTW3 Fortran 77 wrapper to Intel(R) oneAPI Math Kernel Library (Intel(R) oneMKL).
 *
 ******************************************************************************
 */

#include "fftw3_mkl_f77.h"
#ifdef DFT_ENABLE_OFFLOAD
#include "oneapi/mkl/export.hpp"
void fftwf_execute_omp_offload(const fftwf_plan plan, void *interopObj);

#ifdef MKL_ILP64
DLL_EXPORT void
sfftw_execute_omp_offload(PLAN *p, void *interopObj)
{
    if (p == NULL) return;
    fftwf_execute_omp_offload(*(fftwf_plan *)p, interopObj);
}
#endif
#else
void
sfftw_execute(PLAN *p)
{
    if (p == NULL) return;
    fftwf_execute(*(fftwf_plan *)p);
}
#endif
