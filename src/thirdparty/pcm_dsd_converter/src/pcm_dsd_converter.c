/* DSP port of PCM-DSD_Converter_v2, Copyright (c) 2026 serieril (PCMDSD.com).
   See ../LICENSE. C streaming adapter and radix-2 FFT implementation. */
#include "pcm_dsd_converter.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "coefficients.h"
#include "pcm_dsd_simd.h"
#ifdef PCM_DSD_USE_SIMD
#include <emmintrin.h>
#ifdef _MSC_VER
#include <intrin.h>
#endif
#endif
#ifdef PCM_DSD_USE_MKL
#include <mkl_dfti.h>
#endif
#define BLOCK 256
#define TAPS 4095
#define MAX_STAGES 11
/* Optional section timers supplied only by the profiling harness. */
#ifndef PCM_DSD_PROFILE_BEGIN
#define PCM_DSD_PROFILE_BEGIN() ((void)0)
#define PCM_DSD_PROFILE_END(section) ((void)0)
#endif
typedef struct {
    size_t nfft, count;
    complex64 *filter, *roots;
    uint32_t* reversed;
    double* tail;
#ifdef PCM_DSD_USE_MKL
    DFTI_DESCRIPTOR_HANDLE descriptor;
#endif
} fir_stage;
typedef struct { uint64_t rng; double states[16], feedback, last; uint8_t prev; int64_t run; int32_t byte, bits; } mod_state;
struct pcm_dsd_converter {
    pcm_dsd_config config;
    pcm_dsd_options options;
    size_t taps;
    uint32_t rate, up, stages;
    fir_stage stage[MAX_STAGES];
    complex64* scratch;
    double *a, *b;
    double* input;
    size_t input_count;
    uint8_t* pending;
    size_t pending_count, pending_pos;
    mod_state mod[2];
    void (*multiply)(complex64*, const complex64*, size_t);
    int stereo_simd;
    int flushing;
};
static void multiply_scalar(complex64* values, const complex64* filter, size_t count) {
    size_t i;
    for(i=0;i<count;i++) {
        const complex64 a=values[i],b=filter[i];
        values[i].re=a.re*b.re-a.im*b.im;
        values[i].im=a.re*b.im+a.im*b.re;
    }
}
#ifdef PCM_DSD_USE_SIMD
static int has_avx2(void) {
#ifdef _MSC_VER
    int regs[4];
    __cpuid(regs,0); if(regs[0]<7) return 0;
    __cpuidex(regs,1,0);
    if((regs[2]&0x18000000)!=0x18000000 || (_xgetbv(0)&6)!=6) return 0;
    __cpuidex(regs,7,0); return (regs[1]&(1<<5))!=0;
#else
    __builtin_cpu_init(); return __builtin_cpu_supports("avx2")!=0;
#endif
}
#endif
static size_t power2(size_t n) { size_t p=1; while(p<n) p*=2; return p; }
static void fft(complex64* a, size_t n, int inverse) {
    size_t i,j,k,len;
    for(i=1,j=0;i<n;i++) {
        size_t bit=n>>1;
        for(;j&bit;bit>>=1) j^=bit;
        j^=bit;
        if(i<j) { complex64 t=a[i]; a[i]=a[j]; a[j]=t; }
    }
    for(len=2;len<=n;len*=2) {
        double angle=(inverse?2.0:-2.0)*3.1415926535897932384626433832795/(double)len;
        double wr=cos(angle), wi=sin(angle);
        for(i=0;i<n;i+=len) {
            double r=1.0,im=0.0;
            for(k=0;k<len/2;k++) {
                complex64 u=a[i+k], b=a[i+k+len/2];
                double vr=b.re*r-b.im*im, vi=b.re*im+b.im*r;
                double nr=r*wr-im*wi;
                a[i+k].re=u.re+vr; a[i+k].im=u.im+vi;
                a[i+k+len/2].re=u.re-vr; a[i+k+len/2].im=u.im-vi;
                im=r*wi+im*wr; r=nr;
            }
        }
    }
    if(inverse) for(i=0;i<n;i++) { a[i].re/=(double)n; a[i].im/=(double)n; }
}
/* Per-converter immutable FFT plan. Avoid recomputing bit reversal and the
   serial twiddle recurrence in every playback block. */
static int planned_fft(complex64* a, const fir_stage* plan, int inverse) {
#ifdef PCM_DSD_USE_MKL
    return (inverse ? DftiComputeBackward(plan->descriptor,a) :
        DftiComputeForward(plan->descriptor,a)) == DFTI_NO_ERROR;
#else
    const size_t n=plan->nfft;
    size_t i,k,len;
    for(i=0;i<n;i++) if(i<plan->reversed[i]) {
        complex64 t=a[i]; a[i]=a[plan->reversed[i]]; a[plan->reversed[i]]=t;
    }
    for(len=2;len<=n;len*=2) {
        const size_t step=n/len;
        for(i=0;i<n;i+=len) for(k=0;k<len/2;k++) {
            const complex64 w=plan->roots[k*step],u=a[i+k],b=a[i+k+len/2];
            const double wi=inverse?-w.im:w.im;
            const double vr=b.re*w.re-b.im*wi,vi=b.re*wi+b.im*w.re;
            a[i+k].re=u.re+vr; a[i+k].im=u.im+vi;
            a[i+k+len/2].re=u.re-vr; a[i+k+len/2].im=u.im-vi;
        }
    }
    if(inverse) for(i=0;i<n;i++) { a[i].re/=(double)n; a[i].im/=(double)n; }
    return 1;
#endif
}
static uint8_t modulate(double sample, const double* sos, double gain, double amplitude, mod_state* m) {
    double d=0.0, q, ret, x, sum=0.0;
    uint8_t bit;
    int s;
    if(amplitude!=0.0) {
        m->rng=m->rng*UINT64_C(6364136223846793005)+UINT64_C(1442695040888963407);
        d=(((double)(m->rng>>11)*(1.0/9007199254740992.0))*2.0-1.0)*amplitude;
    }
    q=sample*gain+d+m->feedback;
    bit=(uint8_t)(q>=0.0);
    ret=q+1.02*(bit ? -1.0-d : 1.0-d);
    if(fabs(ret)>3.0) { ret*=0.75; for(s=0;s<16;s++) m->states[s]*=0.65; }
    x=ret;
    for(s=0;s<8;s++) {
        const double* c=sos+s*6;
        double y=x+m->states[s*2];
        m->states[s*2]=c[1]*x-c[4]*y+m->states[s*2+1];
        m->states[s*2+1]=c[2]*x-c[5]*y;
        x=y;
        sum+=m->states[s*2];
    }
    m->feedback=-sum;
    m->run=(m->run>0 && bit==m->prev) ? (m->run<INT64_MAX?m->run+1:INT64_MAX) : 1;
    m->prev=bit; m->last=ret;
    return bit;
}
/* First-order error-feedback sigma-delta. Clamp overload to avoid integrator
   windup; this low-cost mode has substantially more in-band noise than order 15. */
static uint8_t modulate_first(double sample, double gain, double amplitude, mod_state* m) {
    double d=0.0,x,q; uint8_t bit;
    if(amplitude!=0.0) {
        m->rng=m->rng*UINT64_C(6364136223846793005)+UINT64_C(1442695040888963407);
        d=(((double)(m->rng>>11)*(1.0/9007199254740992.0))*2.0-1.0)*amplitude;
    }
    x=sample*gain+d;
    if(x>1.0) x=1.0; else if(x< -1.0) x=-1.0;
    q=x+m->feedback; bit=(uint8_t)(q>=0.0);
    m->feedback=q-(bit?1.0:-1.0);
    return bit;
}
#ifdef PCM_DSD_USE_SIMD
/* Independent L/R lanes; samples and SOS sections remain sequential.
   The public legacy modulator keeps its diagnostic state updates unchanged. */
static void modulate_stereo15(pcm_dsd_converter* c, const double* input, size_t count) {
    __m128d state[16],coeff[8][4];
    __m128d feedback=_mm_set_pd(c->mod[1].feedback,c->mod[0].feedback);
    const __m128d gain=_mm_set1_pd(c->config.input_gain);
    const __m128d amplitude=_mm_set1_pd(c->config.dither_amplitude);
    const __m128d one=_mm_set1_pd(1.0),negative=_mm_set1_pd(-1.0);
    const __m128d sign=_mm_set1_pd(-0.0),zero=_mm_setzero_pd();
    const int dither=c->config.dither_amplitude!=0.0;
    uint64_t left_rng=c->mod[0].rng,right_rng=c->mod[1].rng;
    unsigned left_byte=(unsigned)c->mod[0].byte,right_byte=(unsigned)c->mod[1].byte;
    int bits=c->mod[0].bits,s;
    size_t i;
    for(s=0;s<16;s++) state[s]=_mm_set_pd(c->mod[1].states[s],c->mod[0].states[s]);
    for(s=0;s<8;s++) {
        coeff[s][0]=_mm_set1_pd(default_sos[s*6+1]);
        coeff[s][1]=_mm_set1_pd(default_sos[s*6+4]);
        coeff[s][2]=_mm_set1_pd(default_sos[s*6+2]);
        coeff[s][3]=_mm_set1_pd(default_sos[s*6+5]);
    }
    for(i=0;i<count;i++) {
        __m128d d=zero,q,mask,ret,x,sum=zero;
        int bitmask;
        if(dither) {
            left_rng=left_rng*UINT64_C(6364136223846793005)+UINT64_C(1442695040888963407);
            right_rng=right_rng*UINT64_C(6364136223846793005)+UINT64_C(1442695040888963407);
            d=_mm_set_pd((double)(right_rng>>11),(double)(left_rng>>11));
            d=_mm_mul_pd(d,_mm_set1_pd(1.0/9007199254740992.0));
            d=_mm_mul_pd(_mm_sub_pd(_mm_mul_pd(d,_mm_set1_pd(2.0)),one),amplitude);
        }
        q=_mm_add_pd(_mm_add_pd(_mm_mul_pd(_mm_loadu_pd(input+i*2),gain),d),feedback);
        mask=_mm_cmpge_pd(q,zero);
        bitmask=_mm_movemask_pd(mask);
        ret=_mm_or_pd(_mm_and_pd(mask,negative),_mm_andnot_pd(mask,one));
        ret=_mm_add_pd(q,_mm_mul_pd(_mm_set1_pd(1.02),_mm_sub_pd(ret,d)));
        mask=_mm_cmpgt_pd(_mm_andnot_pd(sign,ret),_mm_set1_pd(3.0));
        if(_mm_movemask_pd(mask)) {
            const __m128d scale=_mm_or_pd(_mm_and_pd(mask,_mm_set1_pd(0.65)),_mm_andnot_pd(mask,one));
            ret=_mm_mul_pd(ret,_mm_or_pd(_mm_and_pd(mask,_mm_set1_pd(0.75)),_mm_andnot_pd(mask,one)));
            for(s=0;s<16;s++) state[s]=_mm_mul_pd(state[s],scale);
        }
        x=ret;
        for(s=0;s<8;s++) {
            const __m128d y=_mm_add_pd(x,state[s*2]);
            state[s*2]=_mm_add_pd(_mm_sub_pd(_mm_mul_pd(coeff[s][0],x),_mm_mul_pd(coeff[s][1],y)),state[s*2+1]);
            state[s*2+1]=_mm_sub_pd(_mm_mul_pd(coeff[s][2],x),_mm_mul_pd(coeff[s][3],y));
            x=y; sum=_mm_add_pd(sum,state[s*2]);
        }
        feedback=_mm_xor_pd(sum,sign);
        left_byte=(left_byte<<1)|(unsigned)(bitmask&1);
        right_byte=(right_byte<<1)|(unsigned)((bitmask>>1)&1);
        if(++bits==8) {
            c->pending[c->pending_count++]=(uint8_t)left_byte;
            c->pending[c->pending_count++]=(uint8_t)right_byte;
            left_byte=right_byte=0; bits=0;
        }
    }
    for(s=0;s<16;s++) {
        _mm_storel_pd(&c->mod[0].states[s],state[s]);
        _mm_storeh_pd(&c->mod[1].states[s],state[s]);
    }
    _mm_storel_pd(&c->mod[0].feedback,feedback); _mm_storeh_pd(&c->mod[1].feedback,feedback);
    c->mod[0].rng=left_rng; c->mod[1].rng=right_rng;
    c->mod[0].byte=(int32_t)left_byte; c->mod[1].byte=(int32_t)right_byte;
    c->mod[0].bits=c->mod[1].bits=bits;
}
#endif
int PCM_DSD_CALL dsd15_modulate_pack(const double* samples, int64_t frames,
    const double* sos, double gain, double dither, uint64_t* rng, double* states,
    double* feedback, uint8_t* previous, int64_t* run, double* last_return,
    int32_t* carry_byte, int32_t* carry_count, uint8_t* output, int64_t capacity, int64_t* written) {
    mod_state m; int64_t i,n=0,need;
    if(written) *written=0;
    if(!written || !sos || !rng || !states || !feedback || !previous || !run ||
       !last_return || !carry_byte || !carry_count || frames<0 || capacity<0 ||
       (frames && !samples) || *carry_count<0 || *carry_count>7 ||
       *carry_byte<0 || *carry_byte >= (1<<*carry_count) || !isfinite(gain) || !isfinite(dither) || dither<0)
        return PCM_DSD_INVALID_ARGUMENT;
    need=frames/8+(frames%8+*carry_count)/8;
    if(capacity<need || (need && !output)) return PCM_DSD_INVALID_ARGUMENT;
    m.rng=*rng; memcpy(m.states,states,sizeof(m.states)); m.feedback=*feedback;
    m.prev=*previous; m.run=*run; m.last=*last_return; m.byte=*carry_byte; m.bits=*carry_count;
    for(i=0;i<frames;i++) {
        m.byte=(m.byte<<1)|modulate(samples[i],sos,gain,dither,&m);
        if(++m.bits==8) { output[n++]=(uint8_t)m.byte; m.byte=m.bits=0; }
    }
    *rng=m.rng; memcpy(states,m.states,sizeof(m.states)); *feedback=m.feedback;
    *previous=m.prev; *run=m.run; *last_return=m.last; *carry_byte=m.byte; *carry_count=m.bits;
    *written=n; return PCM_DSD_OK;
}
void PCM_DSD_CALL pcm_dsd_default_config(pcm_dsd_config* c) {
    if(!c) return;
    memset(c,0,sizeof(*c)); c->struct_size=sizeof(*c); c->input_sample_rate=44100;
    c->channels=2; c->dsd_multiplier=64; c->input_gain=0.5;
    c->dither_amplitude=0x1p-24; c->seed=20260513;
}
void PCM_DSD_CALL pcm_dsd_destroy(pcm_dsd_converter* c) {
    uint32_t s; if(!c) return;
    for(s=0;s<MAX_STAGES;s++) {
#ifdef PCM_DSD_USE_MKL
        if(c->stage[s].descriptor) DftiFreeDescriptor(&c->stage[s].descriptor);
#endif
        free(c->stage[s].filter); free(c->stage[s].tail);
        free(c->stage[s].roots); free(c->stage[s].reversed);
    }
    free(c->input); free(c->scratch); free(c->a); free(c->b); free(c->pending); free(c);
}
int PCM_DSD_CALL pcm_dsd_reset(pcm_dsd_converter* c) {
    uint32_t s; if(!c) return PCM_DSD_INVALID_ARGUMENT;
    for(s=0;s<c->stages;s++) memset(c->stage[s].tail,0,(c->taps-1)*2*sizeof(double));
    memset(c->mod,0,sizeof(c->mod));
    c->mod[0].rng=c->mod[1].rng=c->config.seed^UINT64_C(0x9E3779B97F4A7C15);
    c->input_count=c->pending_count=c->pending_pos=0; c->flushing=0;
    return PCM_DSD_OK;
}
void PCM_DSD_CALL pcm_dsd_default_options(pcm_dsd_options* o) {
    if(!o) return;
    memset(o,0,sizeof(*o)); o->struct_size=sizeof(*o); o->block_frames=BLOCK;
}
int PCM_DSD_CALL pcm_dsd_create(const pcm_dsd_config* config, pcm_dsd_converter** out) {
    pcm_dsd_options options; pcm_dsd_default_options(&options);
    return pcm_dsd_create_ex(config,&options,out);
}
int PCM_DSD_CALL pcm_dsd_create_ex(const pcm_dsd_config* config, const pcm_dsd_options* options, pcm_dsd_converter** out) {
    pcm_dsd_converter* c; uint32_t base,up,s; size_t n=BLOCK,maxfft=1,i;
    if(!out) return PCM_DSD_INVALID_ARGUMENT;
    *out=NULL;
    if(!options || options->struct_size!=sizeof(*options) || options->filter>PCM_DSD_FIR_BLACKMAN_511 ||
       options->modulator>PCM_DSD_MODULATOR_1 || options->block_frames<64 || options->block_frames>1024 ||
       (options->block_frames&(options->block_frames-1))) return PCM_DSD_INVALID_ARGUMENT;
    n=options->block_frames;
    if(!config || config->struct_size!=sizeof(*config) || !config->input_sample_rate ||
       config->channels<1 || config->channels>2 || config->dsd_multiplier<16 || config->dsd_multiplier>2048 ||
       (config->dsd_multiplier&(config->dsd_multiplier-1)) || !isfinite(config->input_gain) ||
       !isfinite(config->dither_amplitude) || config->dither_amplitude<0) return PCM_DSD_INVALID_ARGUMENT;
    base=config->input_sample_rate%44100==0 ? 44100 : (config->input_sample_rate%48000==0 ? 48000 : 0);
    if(!base || (base*config->dsd_multiplier)%config->input_sample_rate) return PCM_DSD_INVALID_ARGUMENT;
    up=base*config->dsd_multiplier/config->input_sample_rate;
    if(!up || (up&(up-1))) return PCM_DSD_INVALID_ARGUMENT;
    c=(pcm_dsd_converter*)calloc(1,sizeof(*c)); if(!c) return PCM_DSD_OUT_OF_MEMORY;
    c->config=*config; c->options=*options;
    c->multiply=multiply_scalar;
#ifdef PCM_DSD_USE_SIMD
    c->stereo_simd=1;
    if(has_avx2()) c->multiply=pcm_dsd_multiply_avx2;
#endif
    c->taps=options->filter==PCM_DSD_FIR_ORIGINAL ? TAPS :
        (options->filter==PCM_DSD_FIR_BLACKMAN_1023 ? 1023 : 511);
    c->input=(double*)calloc(n*2,sizeof(double));
    if(!c->input) { pcm_dsd_destroy(c); return PCM_DSD_OUT_OF_MEMORY; }
    c->rate=base*config->dsd_multiplier; c->up=up;
    while(up>1) { c->stages++; up/=2; }
    for(s=0;s<c->stages;s++) {
        fir_stage* st=&c->stage[s]; n*=2; st->count=n; st->nfft=power2(n+c->taps-1); maxfft=st->nfft;
        st->filter=(complex64*)calloc(st->nfft,sizeof(complex64));
#ifndef PCM_DSD_USE_MKL
        st->roots=(complex64*)calloc(st->nfft/2,sizeof(complex64));
        st->reversed=(uint32_t*)calloc(st->nfft,sizeof(uint32_t));
        st->tail=(double*)calloc((c->taps-1)*2,sizeof(double));
        if(!st->filter || !st->tail || !st->roots || !st->reversed) { pcm_dsd_destroy(c); return PCM_DSD_OUT_OF_MEMORY; }
        for(i=0;i<st->nfft/2;i++) {
            const double angle=-2.0*3.1415926535897932384626433832795*(double)i/(double)st->nfft;
            st->roots[i].re=cos(angle); st->roots[i].im=sin(angle);
        }
        for(i=1;i<st->nfft;i++) st->reversed[i]=(st->reversed[i>>1]>>1)|((i&1)?(uint32_t)(st->nfft>>1):0);
#else
        st->tail=(double*)calloc((c->taps-1)*2,sizeof(double));
        if(!st->filter || !st->tail) { pcm_dsd_destroy(c); return PCM_DSD_OUT_OF_MEMORY; }
        if(DftiCreateDescriptor(&st->descriptor,DFTI_DOUBLE,DFTI_COMPLEX,1,(MKL_LONG)st->nfft) != DFTI_NO_ERROR ||
           DftiSetValue(st->descriptor,DFTI_BACKWARD_SCALE,1.0/(double)st->nfft) != DFTI_NO_ERROR ||
           DftiCommitDescriptor(st->descriptor) != DFTI_NO_ERROR) {
            pcm_dsd_destroy(c); return PCM_DSD_INVALID_STATE;
        }
#endif
        if(options->filter==PCM_DSD_FIR_ORIGINAL) {
            for(i=0;i<c->taps;i++) st->filter[i].re=2.0*fir[i];
        } else {
            /* Blackman-windowed half-band sinc, cutoff at half Nyquist.
               Normalize DC gain to two for zero-insertion interpolation. */
            double sum=0.0;
            const double pi=3.1415926535897932384626433832795;
            for(i=0;i<c->taps;i++) {
                const double x=(double)i-(double)(c->taps-1)/2.0;
                const double phase=2.0*pi*(double)i/(double)(c->taps-1);
                const double window=0.42-0.5*cos(phase)+0.08*cos(2.0*phase);
                const double value=(x==0.0 ? 0.5 : sin(0.5*pi*x)/(pi*x))*window;
                st->filter[i].re=value; sum+=value;
            }
            for(i=0;i<c->taps;i++) st->filter[i].re*=2.0/sum;
        }
        if(!planned_fft(st->filter,st,0)) { pcm_dsd_destroy(c); return PCM_DSD_INVALID_STATE; }
    }
    c->scratch=(complex64*)calloc(maxfft,sizeof(complex64));
    c->a=(double*)calloc(n*2,sizeof(double)); c->b=(double*)calloc(n*2,sizeof(double));
    c->pending=(uint8_t*)malloc((n/8+1)*2);
    if(!c->scratch || !c->a || !c->b || !c->pending) { pcm_dsd_destroy(c); return PCM_DSD_OUT_OF_MEMORY; }
    pcm_dsd_reset(c); *out=c; return PCM_DSD_OK;
}
/* Fixed-size overlap-add per 2x stage: deterministic across caller chunk sizes. */
static int convert_block(pcm_dsd_converter* c, size_t valid) {
    size_t n=c->options.block_frames,i; uint32_t s,ch; double *in=c->a,*out=c->b;
    PCM_DSD_PROFILE_BEGIN();
    memcpy(in,c->input,valid*2*sizeof(double)); memset(in+valid*2,0,(c->options.block_frames-valid)*2*sizeof(double));
    PCM_DSD_PROFILE_END(block_copy);
    for(s=0;s<c->stages;s++) {
        fir_stage* st=&c->stage[s];
        /* The FIR is real, so one complex convolution processes L+iR. */
            memset(c->scratch,0,st->nfft*sizeof(complex64));
            for(i=0;i<n;i++) {
                c->scratch[i*2].re=in[i*2]; c->scratch[i*2].im=in[i*2+1];
            }
            PCM_DSD_PROFILE_END(zero_insert);
            if(!planned_fft(c->scratch,st,0)) return 0;
            PCM_DSD_PROFILE_END(forward_fft);
            c->multiply(c->scratch,st->filter,st->nfft);
            PCM_DSD_PROFILE_END(multiply);
            if(!planned_fft(c->scratch,st,1)) return 0;
            PCM_DSD_PROFILE_END(inverse_fft);
            for(i=0;i<c->taps-1;i++) {
                c->scratch[i].re+=st->tail[i*2]; c->scratch[i].im+=st->tail[i*2+1];
            }
            PCM_DSD_PROFILE_END(overlap);
            for(i=0;i<n*2;i++) { out[i*2]=c->scratch[i].re; out[i*2+1]=c->scratch[i].im; }
            PCM_DSD_PROFILE_END(output_copy);
            for(i=0;i<c->taps-1;i++) { st->tail[i*2]=c->scratch[n*2+i].re; st->tail[i*2+1]=c->scratch[n*2+i].im; }
            PCM_DSD_PROFILE_END(tail_copy);
        { double* t=in; in=out; out=t; } n*=2;
    }
    c->pending_pos=c->pending_count=0;
#ifdef PCM_DSD_USE_SIMD
    if(c->stereo_simd && c->options.modulator==PCM_DSD_MODULATOR_15) {
        modulate_stereo15(c,in,valid*c->up);
    } else
#endif
    {
    for(i=0;i<valid*c->up;i++) {
        for(ch=0;ch<2;ch++) {
            mod_state* m=&c->mod[ch];
            const uint8_t bit=c->options.modulator==PCM_DSD_MODULATOR_1
                ? modulate_first(in[i*2+ch],c->config.input_gain,c->config.dither_amplitude,m)
                : modulate(in[i*2+ch],default_sos,c->config.input_gain,c->config.dither_amplitude,m);
            m->byte=(m->byte<<1)|bit;
            m->bits++;
        }
        if(c->mod[0].bits==8) for(ch=0;ch<2;ch++) {
            c->pending[c->pending_count++]=(uint8_t)c->mod[ch].byte;
            c->mod[ch].byte=c->mod[ch].bits=0;
        }
    }
    }
    c->input_count=0;
    PCM_DSD_PROFILE_END(modulate_pack);
    return 1;
}
static size_t drain(pcm_dsd_converter* c, uint8_t* out, size_t capacity) {
    size_t n=c->pending_count-c->pending_pos;
    PCM_DSD_PROFILE_BEGIN();
    capacity-=capacity%2; if(n>capacity) n=capacity;
    if(n) memcpy(out,c->pending+c->pending_pos,n);
    c->pending_pos+=n;
    PCM_DSD_PROFILE_END(drain_copy);
    return n;
}
int PCM_DSD_CALL pcm_dsd_process(pcm_dsd_converter* c, const double* pcm, size_t frames,
    uint8_t* output, size_t capacity, size_t* consumed, size_t* written) {
    size_t used=0,count=0,i,take; uint32_t ch;
    if(consumed) *consumed=0; if(written) *written=0;
    if(!c || !consumed || !written || !output || capacity<2 || (frames && !pcm) ||
       frames>SIZE_MAX/(2*sizeof(double))) return PCM_DSD_INVALID_ARGUMENT;
    if(c->flushing) return PCM_DSD_INVALID_STATE;
    for(;;) {
        count+=drain(c,output+count,capacity-count);
        if(c->pending_pos<c->pending_count || used==frames || capacity-count<2) break;
        take=c->options.block_frames-c->input_count; if(take>frames-used) take=frames-used;
        {
        PCM_DSD_PROFILE_BEGIN();
        for(i=0;i<take;i++) for(ch=0;ch<2;ch++) {
            double v=pcm[(used+i)*c->config.channels+(c->config.channels==1?0:ch)];
            if(!isfinite(v)) v=isnan(v)?0.0:(v>0?1.0:-1.0);
            c->input[(c->input_count+i)*2+ch]=v;
        }
        PCM_DSD_PROFILE_END(input_copy);
        }
        used+=take; c->input_count+=take;
        if(c->input_count==c->options.block_frames && !convert_block(c,c->options.block_frames)) {
            c->flushing=1; return PCM_DSD_INVALID_STATE;
        }
    }
    *consumed=used; *written=count; return PCM_DSD_OK;
}
int PCM_DSD_CALL pcm_dsd_flush(pcm_dsd_converter* c, uint8_t* output, size_t capacity, size_t* written) {
    size_t count; uint32_t ch;
    if(written) *written=0;
    if(!c || !output || capacity<2 || !written) return PCM_DSD_INVALID_ARGUMENT;
    c->flushing=1; count=drain(c,output,capacity);
    if(c->pending_pos==c->pending_count && c->input_count) {
        if(!convert_block(c,c->input_count)) return PCM_DSD_INVALID_STATE;
        if(c->mod[0].bits) for(ch=0;ch<2;ch++) {
            c->pending[c->pending_count++]=(uint8_t)(c->mod[ch].byte<<(8-c->mod[ch].bits));
            c->mod[ch].byte=c->mod[ch].bits=0;
        }
        count+=drain(c,output+count,capacity-count);
    }
    *written=count;
    return c->pending_pos==c->pending_count && !c->input_count ? PCM_DSD_FINISHED : PCM_DSD_OK;
}
uint32_t PCM_DSD_CALL pcm_dsd_output_sample_rate(const pcm_dsd_converter* c) { return c?c->rate:0; }
const char* PCM_DSD_CALL pcm_dsd_status_string(int status) {
    switch(status) {
    case PCM_DSD_OK: return "OK";
    case PCM_DSD_INVALID_ARGUMENT: return "Invalid argument or insufficient output capacity";
    case PCM_DSD_OUT_OF_MEMORY: return "Out of memory";
    case PCM_DSD_FINISHED: return "Finished";
    case PCM_DSD_INVALID_STATE: return "Reset required after flush";
    default: return "Unknown status";
    }
}
