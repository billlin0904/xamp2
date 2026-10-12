#ifndef PCM_DSD_NEON_H
#define PCM_DSD_NEON_H
#include <arm_neon.h>

/* AArch64 FP64 lanes hold independent left/right channels. Do not fuse
   multiply/add operations: feedback state must match the scalar reference. */
static void multiply_neon(complex64* values, const complex64* filter, size_t count) {
    size_t i=0;
    for(;count-i>=2;i+=2) {
        const float64x2x2_t a=vld2q_f64((const double*)(values+i));
        const float64x2x2_t b=vld2q_f64((const double*)(filter+i));
        float64x2x2_t out;
        out.val[0]=vsubq_f64(vmulq_f64(a.val[0],b.val[0]),vmulq_f64(a.val[1],b.val[1]));
        out.val[1]=vaddq_f64(vmulq_f64(a.val[0],b.val[1]),vmulq_f64(a.val[1],b.val[0]));
        vst2q_f64((double*)(values+i),out);
    }
    multiply_scalar(values+i,filter+i,count-i);
}

static void modulate_stereo15(pcm_dsd_converter* c, const double* input, size_t count) {
    float64x2_t state[16],coeff[8][4];
    float64x2_t feedback={c->mod[0].feedback,c->mod[1].feedback};
    const float64x2_t gain=vdupq_n_f64(c->config.input_gain);
    const float64x2_t amplitude=vdupq_n_f64(c->config.dither_amplitude);
    const float64x2_t one=vdupq_n_f64(1.0),zero=vdupq_n_f64(0.0);
    const int dither=c->config.dither_amplitude!=0.0;
    uint64_t left_rng=c->mod[0].rng,right_rng=c->mod[1].rng;
    unsigned left_byte=(unsigned)c->mod[0].byte,right_byte=(unsigned)c->mod[1].byte;
    int bits=c->mod[0].bits,s;
    size_t i;
    for(s=0;s<16;s++) state[s]=(float64x2_t){c->mod[0].states[s],c->mod[1].states[s]};
    for(s=0;s<8;s++) {
        coeff[s][0]=vdupq_n_f64(default_sos[s*6+1]);
        coeff[s][1]=vdupq_n_f64(default_sos[s*6+4]);
        coeff[s][2]=vdupq_n_f64(default_sos[s*6+2]);
        coeff[s][3]=vdupq_n_f64(default_sos[s*6+5]);
    }
    for(i=0;i<count;i++) {
        float64x2_t d=zero,q,ret,x,sum=zero;
        uint64x2_t mask;
        if(dither) {
            left_rng=left_rng*UINT64_C(6364136223846793005)+UINT64_C(1442695040888963407);
            right_rng=right_rng*UINT64_C(6364136223846793005)+UINT64_C(1442695040888963407);
            d=(float64x2_t){(double)(left_rng>>11),(double)(right_rng>>11)};
            d=vmulq_n_f64(d,1.0/9007199254740992.0);
            d=vmulq_f64(vsubq_f64(vmulq_n_f64(d,2.0),one),amplitude);
        }
        q=vaddq_f64(vaddq_f64(vmulq_f64(vld1q_f64(input+i*2),gain),d),feedback);
        mask=vcgeq_f64(q,zero);
        const unsigned left_bit=(unsigned)(vgetq_lane_u64(mask,0)&1);
        const unsigned right_bit=(unsigned)(vgetq_lane_u64(mask,1)&1);
        ret=vbslq_f64(mask,vdupq_n_f64(-1.0),one);
        ret=vaddq_f64(q,vmulq_n_f64(vsubq_f64(ret,d),1.02));
        mask=vcgtq_f64(vabsq_f64(ret),vdupq_n_f64(3.0));
        if(vgetq_lane_u64(mask,0)|vgetq_lane_u64(mask,1)) {
            const float64x2_t scale=vbslq_f64(mask,vdupq_n_f64(0.65),one);
            ret=vmulq_f64(ret,vbslq_f64(mask,vdupq_n_f64(0.75),one));
            for(s=0;s<16;s++) state[s]=vmulq_f64(state[s],scale);
        }
        x=ret;
        for(s=0;s<8;s++) {
            const float64x2_t y=vaddq_f64(x,state[s*2]);
            state[s*2]=vaddq_f64(vsubq_f64(vmulq_f64(coeff[s][0],x),vmulq_f64(coeff[s][1],y)),state[s*2+1]);
            state[s*2+1]=vsubq_f64(vmulq_f64(coeff[s][2],x),vmulq_f64(coeff[s][3],y));
            x=y; sum=vaddq_f64(sum,state[s*2]);
        }
        feedback=vnegq_f64(sum);
        left_byte=(left_byte<<1)|left_bit;
        right_byte=(right_byte<<1)|right_bit;
        if(++bits==8) {
            c->pending[c->pending_count++]=(uint8_t)left_byte;
            c->pending[c->pending_count++]=(uint8_t)right_byte;
            left_byte=right_byte=0; bits=0;
        }
    }
    for(s=0;s<16;s++) {
        c->mod[0].states[s]=vgetq_lane_f64(state[s],0);
        c->mod[1].states[s]=vgetq_lane_f64(state[s],1);
    }
    c->mod[0].feedback=vgetq_lane_f64(feedback,0); c->mod[1].feedback=vgetq_lane_f64(feedback,1);
    c->mod[0].rng=left_rng; c->mod[1].rng=right_rng;
    c->mod[0].byte=(int32_t)left_byte; c->mod[1].byte=(int32_t)right_byte;
    c->mod[0].bits=c->mod[1].bits=bits;
}
#endif
