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
!      FFTW3 interface for OpenMP target (offload)
!******************************************************************************/


#ifndef FFTW3_OMP_OFFLOAD_H_INCLUDED
#define FFTW3_OMP_OFFLOAD_H_INCLUDED

#include "../fftw3.h"
#include "mkl_dfti_omp_offload.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Double precision offload function declarations
extern fftw_plan fftw_plan_dft_1d_omp_offload(int n0, fftw_complex *in,
                                              fftw_complex *out, int sign,
                                              unsigned flags);

extern fftw_plan fftw_plan_dft_2d_omp_offload(int n0, int n1, fftw_complex *in,
                                              fftw_complex *out, int sign,
                                              unsigned flags);

extern fftw_plan fftw_plan_dft_3d_omp_offload(int n0, int n1, int n2,
                                              fftw_complex *in, fftw_complex *out,
                                              int sign, unsigned flags);

extern fftw_plan fftw_plan_dft_omp_offload(int rank, const int *n,
                                           fftw_complex *in, fftw_complex *out,
                                           int sign, unsigned flags);

extern fftw_plan fftw_plan_many_dft_omp_offload(int rank, const int *n, int howmany,
                                                fftw_complex *in, const int *inembed,
                                                int istride, int idist,
                                                fftw_complex *out, const int *onembed,
                                                int ostride, int odist, int sign,
                                                unsigned flags);

extern fftw_plan fftw_plan_guru_dft_omp_offload(int rank, const fftw_iodim *dims,
                                                int howmany_rank, const fftw_iodim
                                                *howmany_dims, fftw_complex *in,
                                                fftw_complex *out, int sign,
                                                unsigned flags);

extern fftw_plan fftw_plan_guru64_dft_omp_offload(int rank, const fftw_iodim64 *dims,
                                                  int howmany_rank, const fftw_iodim64
                                                  *howmany_dims, fftw_complex *in,
                                                  fftw_complex *out, int sign,
                                                  unsigned flags);

extern void fftw_execute_omp_offload(const fftw_plan plan);
extern void fftw_execute_dft_omp_offload(const fftw_plan plan, fftw_complex *in,
                                         fftw_complex *out, void* interopObj);

// Single precision offload function declarations
extern fftwf_plan fftwf_plan_dft_1d_omp_offload(int n0, fftwf_complex *in,
                                                fftwf_complex *out, int sign,
                                                unsigned flags);

extern fftwf_plan fftwf_plan_dft_2d_omp_offload(int n0, int n1, fftwf_complex *in,
                                                fftwf_complex *out, int sign,
                                                unsigned flags);

extern fftwf_plan fftwf_plan_dft_3d_omp_offload(int n0, int n1, int n2,
                                                fftwf_complex *in, fftwf_complex *out,
                                                int sign, unsigned flags);

extern fftwf_plan fftwf_plan_dft_omp_offload(int rank, const int *n,
                                             fftwf_complex *in, fftwf_complex *out,
                                             int sign, unsigned flags);

extern fftwf_plan fftwf_plan_many_dft_omp_offload(int rank, const int *n, int howmany,
                                                  fftwf_complex *in, const int *inembed,
                                                  int istride, int idist,
                                                  fftwf_complex *out, const int *onembed,
                                                  int ostride, int odist, int sign,
                                                  unsigned flags);

extern fftwf_plan fftwf_plan_guru_dft_omp_offload(int rank, const fftwf_iodim *dims,
                                                  int howmany_rank, const fftwf_iodim
                                                  *howmany_dims, fftwf_complex *in,
                                                  fftwf_complex *out, int sign,
                                                  unsigned flags);

extern fftwf_plan fftwf_plan_guru64_dft_omp_offload(int rank, const fftwf_iodim64 *dims,
                                                    int howmany_rank, const fftwf_iodim64
                                                    *howmany_dims, fftwf_complex *in,
                                                    fftwf_complex *out, int sign,
                                                    unsigned flags);

extern void fftwf_execute_omp_offload(const fftwf_plan plan);
extern void fftwf_execute_dft_omp_offload(const fftwf_plan plan, fftwf_complex *in,
                                          fftwf_complex *out, void* interopObj);


// Double precision variant function declarations for the standard fftw routines
#pragma omp declare variant (fftw_plan_dft_1d_omp_offload) match(construct={target variant dispatch}, device={arch(gen)})
extern fftw_plan fftw_plan_dft_1d(int n0, fftw_complex *in, fftw_complex *out, int sign, unsigned flags);

#pragma omp declare variant (fftw_plan_dft_2d_omp_offload) match(construct={target variant dispatch}, device={arch(gen)})
extern fftw_plan fftw_plan_dft_2d(int n0, int n1, fftw_complex *in, fftw_complex *out, int sign, unsigned flags);

#pragma omp declare variant (fftw_plan_dft_3d_omp_offload) match(construct={target variant dispatch}, device={arch(gen)})
extern fftw_plan fftw_plan_dft_3d(int n0, int n1, int n2, fftw_complex *in, fftw_complex *out, int sign, unsigned flags);

#pragma omp declare variant (fftw_plan_dft_omp_offload) match(construct={target variant dispatch}, device={arch(gen)})
extern fftw_plan fftw_plan_dft(int rank, const int *n, fftw_complex *in, fftw_complex *out, int sign, unsigned flags);

#pragma omp declare variant (fftw_plan_many_dft_omp_offload) match(construct={target variant dispatch}, device={arch(gen)})
extern fftw_plan fftw_plan_many_dft(int rank, const int *n, int howmany, fftw_complex *in, const int *inembed, int istride, int idist,
                                    fftw_complex *out, const int *onembed, int ostride, int odist, int sign, unsigned flags);

#pragma omp declare variant (fftw_plan_guru_dft_omp_offload) match(construct={target variant dispatch}, device={arch(gen)})
extern fftw_plan fftw_plan_guru_dft(int rank, const fftw_iodim *dims, int howmany_rank, const fftw_iodim *howmany_dims,
                                    fftw_complex *in, fftw_complex *out, int sign, unsigned flags);

#pragma omp declare variant (fftw_execute_omp_offload) match(construct={target variant dispatch}, device={arch(gen)})
extern void fftw_execute(const fftw_plan plan);

#pragma omp declare variant (fftw_execute_dft_omp_offload) match(construct={target variant dispatch}, device={arch(gen)})
extern void fftw_execute_dft(const fftw_plan plan, fftw_complex *in, fftw_complex *out);

// Single precision variant function declarations for the standard fftwf routines
#pragma omp declare variant (fftwf_plan_dft_1d_omp_offload) match(construct={target variant dispatch}, device={arch(gen)})
extern fftwf_plan fftwf_plan_dft_1d(int n0, fftwf_complex *in, fftwf_complex *out, int sign, unsigned flags);

#pragma omp declare variant (fftwf_plan_dft_2d_omp_offload) match(construct={target variant dispatch}, device={arch(gen)})
extern fftwf_plan fftwf_plan_dft_2d(int n0, int n1, fftwf_complex *in, fftwf_complex *out, int sign, unsigned flags);

#pragma omp declare variant (fftwf_plan_dft_3d_omp_offload) match(construct={target variant dispatch}, device={arch(gen)})
extern fftwf_plan fftwf_plan_dft_3d(int n0, int n1, int n2, fftwf_complex *in, fftwf_complex *out, int sign, unsigned flags);

#pragma omp declare variant (fftwf_plan_dft_omp_offload) match(construct={target variant dispatch}, device={arch(gen)})
extern fftwf_plan fftwf_plan_dft(int rank, const int *n, fftwf_complex *in, fftwf_complex *out, int sign, unsigned flags);

#pragma omp declare variant (fftwf_plan_many_dft_omp_offload) match(construct={target variant dispatch}, device={arch(gen)})
extern fftwf_plan fftwf_plan_many_dft(int rank, const int *n, int howmany, fftwf_complex *in, const int *inembed, int istride, int idist,
                                      fftwf_complex *out, const int *onembed, int ostride, int odist, int sign, unsigned flags);

#pragma omp declare variant (fftwf_plan_guru_dft_omp_offload) match(construct={target variant dispatch}, device={arch(gen)})
extern fftwf_plan fftwf_plan_guru_dft(int rank, const fftwf_iodim *dims, int howmany_rank, const fftwf_iodim *howmany_dims,
                                      fftwf_complex *in, fftwf_complex *out, int sign, unsigned flags);

#pragma omp declare variant (fftwf_execute_omp_offload) match(construct={target variant dispatch}, device={arch(gen)})
extern void fftwf_execute(const fftwf_plan plan);

#pragma omp declare variant (fftwf_execute_dft_omp_offload) match(construct={target variant dispatch}, device={arch(gen)})
extern void fftwf_execute_dft(const fftwf_plan plan, fftwf_complex *in, fftwf_complex *out);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif
