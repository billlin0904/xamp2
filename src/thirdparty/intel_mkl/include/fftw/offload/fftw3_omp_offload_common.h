/*******************************************************************************
* Copyright 2020-2021 Intel Corporation.
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
!  Content:
!      Intel(R) oneAPI Math Kernel Library (Intel(R) oneMKL)
!      FFTW3 internal header for OpenMP target (offload)
!******************************************************************************/

#ifndef FFTW3_OMP_OFFLOAD_COMMON_H
#define FFTW3_OMP_OFFLOAD_COMMON_H

#include <ISO_Fortran_binding.h>
#include "mkl_dfti.h"
#include "mkl_service.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

    // Declare the function mkl_DftiCommitDescriptor_omp_offload here
    // with an extra param(void *) for omp interopObject as the declaration
    // in mkl_dfti_omp_offload.h contains just one param (DFTI_DESCRIPTOR_HANDLE)
    DFTI_EXTERN MKL_LONG mkl_DftiCommitDescriptor_omp_offload(DFTI_DESCRIPTOR_HANDLE, void *);

    // Cannot include mkl_dfti_omp_offload.h for the compute function
    // declarations anymore as there would be two different declarations
    // for the commit function declared above. So declaring the compute
    // functions too.
    DFTI_EXTERN MKL_LONG mkl_DftiComputeForward_omp_offload(DFTI_DESCRIPTOR_HANDLE,
                                                            void *, ...);
    DFTI_EXTERN MKL_LONG mkl_DftiComputeBackward_omp_offload(DFTI_DESCRIPTOR_HANDLE,
                                                             void *, ...);

    extern const int mkl_dfti_is_ilp64;

#ifdef __cplusplus
}
#endif // __cplusplus

fftw_plan fftw_plan_dft_omp_offload_impl(int rank, const int *n, fftw_complex *in,
                                           fftw_complex *out, int sign, unsigned flags,
                                           void *interopObj);

fftw_plan fftw_plan_guru64_dft_omp_offload_impl_lp64(int rank, const fftw_iodim64 *dims,
                                                     int howmany_rank, const fftw_iodim64
                                                     *howmany_dims, fftw_complex *in,
                                                     fftw_complex *out, int sign,
                                                     unsigned flags, void *interopObj);

fftw_plan fftw_plan_guru64_dft_omp_offload_impl_ilp64(int rank, const fftw_iodim64 *dims,
                                                      int howmany_rank, const fftw_iodim64
                                                      *howmany_dims, fftw_complex *in,
                                                      fftw_complex *out, int sign,
                                                      unsigned flags, void *interopObj);

fftwf_plan fftwf_plan_dft_omp_offload_impl(int rank, const int *n, fftwf_complex *in,
                                             fftwf_complex *out, int sign, unsigned flags,
                                             void *interopObj);

fftwf_plan fftwf_plan_guru64_dft_omp_offload_impl_lp64(int rank, const fftwf_iodim64 *dims,
                                                       int howmany_rank, const fftwf_iodim64
                                                       *howmany_dims, fftwf_complex *in,
                                                       fftwf_complex *out, int sign,
                                                       unsigned flags, void *interopObj);

fftwf_plan fftwf_plan_guru64_dft_omp_offload_impl_ilp64(int rank, const fftwf_iodim64 *dims,
                                                        int howmany_rank, const fftwf_iodim64
                                                        *howmany_dims, fftwf_complex *in,
                                                        fftwf_complex *out, int sign,
                                                        unsigned flags, void *interopObj);

#include "fftw3_mkl_f77.h"

void
dfftw_plan_dft_omp_offload_impl(PLAN *p, INTEGER *rank, INTEGER *n, COMPLEX16 *in,
               COMPLEX16 *out, INTEGER *sign, INTEGER *flags, void *interopObj);

void
sfftw_plan_dft_omp_offload_impl(PLAN *p, INTEGER *rank, INTEGER *n, COMPLEX8 *in,
               COMPLEX8 *out, INTEGER *sign, INTEGER *flags, void *interopObj);

void
dfftw_plan_guru_dft_omp_offload_impl(PLAN *p, INTEGER *rank, INTEGER *n, INTEGER *is,
                    INTEGER *os, INTEGER *howmany_rank, INTEGER *howmany_n,
                    INTEGER *howmany_is, INTEGER *howmany_os, COMPLEX16 *in,
                    COMPLEX16 *out, INTEGER *sign, INTEGER *flags, void *interopObj);

void
sfftw_plan_guru_dft_omp_offload_impl(PLAN *p, INTEGER *rank, INTEGER *n, INTEGER *is,
                    INTEGER *os, INTEGER *howmany_rank, INTEGER *howmany_n,
                    INTEGER *howmany_is, INTEGER *howmany_os, COMPLEX8 *in,
                    COMPLEX8 *out, INTEGER *sign, INTEGER *flags, void *interopObj);

#endif
