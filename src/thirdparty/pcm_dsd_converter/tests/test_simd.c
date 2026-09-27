#define PCM_DSD_BUILD
#include "../src/pcm_dsd_converter.c"
#include <stdio.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)

static int collect(pcm_dsd_converter* c,const double* input,size_t frames,uint8_t* output,size_t* total) {
    size_t pos=0,used,written; int status; uint8_t buffer[130];
    *total=0;
    while(pos<frames) {
        size_t count=frames-pos; if(count>37) count=37;
        status=pcm_dsd_process(c,input+pos*c->config.channels,count,buffer,sizeof(buffer),&used,&written);
        CHECK(status==PCM_DSD_OK && (used || written));
        pos+=used; memcpy(output+*total,buffer,written); *total+=written;
    }
    do {
        status=pcm_dsd_flush(c,buffer,sizeof(buffer),&written);
        CHECK(status==PCM_DSD_OK || status==PCM_DSD_FINISHED);
        memcpy(output+*total,buffer,written); *total+=written;
    } while(status!=PCM_DSD_FINISHED);
    return 0;
}

int main(void) {
#ifdef PCM_DSD_USE_SIMD
    pcm_dsd_config cfg; pcm_dsd_options opts;
    double input[2006]; uint8_t a[65536],b[65536];
    unsigned dither,mode,filter,block,channels,rate; size_t i,n,m;
    /* Exercise odd counts, unaligned SIMD addresses, and scalar tail. */
    if(has_avx2()) {
        complex64 v[67],ref[67],h[67];
        for(i=0;i<67;i++) {
            v[i].re=sin(i*0.03); v[i].im=cos(i*0.07);
            h[i].re=sin(i*0.11); h[i].im=-cos(i*0.17);
        }
        for(n=0;n<=65;n++) {
            memcpy(ref,v,sizeof(v));
            complex64 actual[67]; memcpy(actual,v,sizeof(v));
            multiply_scalar(ref+1,h+1,n); pcm_dsd_multiply_avx2(actual+1,h+1,n);
            CHECK(!memcmp(ref,actual,sizeof(ref)));
        }
    }
    /* Independent lane RNG, overload in one lane, and partial-byte carries. */
    for(dither=0;dither<2;dither++) {
        pcm_dsd_converter c={0}; mod_state ref[2]={{0}};
        uint8_t pending[1024]; size_t k,pos=0;
        c.pending=pending; c.config.input_gain=0.5; c.config.dither_amplitude=dither?0x1p-24:0;
        c.mod[0].rng=ref[0].rng=UINT64_C(987654321);
        c.mod[1].rng=ref[1].rng=UINT64_C(123456789);
        for(k=0;k<100;k++) {
            const size_t count=1+(k*13)%257;
            c.pending_count=0; n=0;
            for(i=0;i<count;i++) {
                input[i*2]=k<10?12.0:0.8*sin((pos+i)*0.031);
                input[i*2+1]=k<10?0.0:-0.4*cos((pos+i)*0.073);
                for(unsigned ch=0;ch<2;ch++) {
                    ref[ch].byte=(ref[ch].byte<<1)|modulate(input[i*2+ch],default_sos,0.5,c.config.dither_amplitude,&ref[ch]);
                    ++ref[ch].bits;
                }
                if(ref[0].bits==8) for(unsigned ch=0;ch<2;ch++) {
                    a[n++]=(uint8_t)ref[ch].byte; ref[ch].byte=ref[ch].bits=0;
                }
            }
            modulate_stereo15(&c,input,count); pos+=count;
            CHECK(c.pending_count==n && !memcmp(a,pending,n));
            for(unsigned ch=0;ch<2;ch++) {
                CHECK(!memcmp(c.mod[ch].states,ref[ch].states,sizeof(ref[ch].states)));
                CHECK(!memcmp(&c.mod[ch].feedback,&ref[ch].feedback,sizeof(double)));
                CHECK(c.mod[ch].rng==ref[ch].rng && c.mod[ch].bits==ref[ch].bits && c.mod[ch].byte==ref[ch].byte);
            }
        }
    }
    for(i=0;i<1003;i++) { input[i*2]=0.8*sin(i*0.031); input[i*2+1]=-0.4*cos(i*0.073); }
    input[0]=NAN; input[1]=INFINITY; input[2]=-INFINITY;
    for(dither=0;dither<2;dither++) for(mode=0;mode<2;mode++) for(filter=0;filter<3;filter++)
    for(block=64;block<=1024;block*=2) for(channels=1;channels<=2;channels++) {
        pcm_dsd_converter *simd=NULL,*scalar=NULL;
        pcm_dsd_default_config(&cfg); pcm_dsd_default_options(&opts);
        cfg.dsd_multiplier=16; cfg.channels=channels; cfg.dither_amplitude=dither?0x1p-24:0;
        opts.block_frames=block; opts.filter=filter; opts.modulator=mode;
        CHECK(pcm_dsd_create_ex(&cfg,&opts,&simd)==0 && pcm_dsd_create_ex(&cfg,&opts,&scalar)==0);
        scalar->stereo_simd=0; scalar->multiply=multiply_scalar;
        CHECK(collect(simd,input,1003,a,&n)==0 && collect(scalar,input,1003,b,&m)==0);
        CHECK(n==m && !memcmp(a,b,n));
        CHECK(pcm_dsd_reset(simd)==0 && collect(simd,input,1003,a,&n)==0 && !memcmp(a,b,n));
        pcm_dsd_destroy(simd); pcm_dsd_destroy(scalar);
    }
    for(rate=64;rate<=256;rate*=2) {
        pcm_dsd_converter *simd=NULL,*scalar=NULL;
        pcm_dsd_default_config(&cfg); cfg.dsd_multiplier=rate;
        CHECK(pcm_dsd_create(&cfg,&simd)==0 && pcm_dsd_create(&cfg,&scalar)==0);
        scalar->stereo_simd=0; scalar->multiply=multiply_scalar;
        CHECK(collect(simd,input,1003,a,&n)==0 && collect(scalar,input,1003,b,&m)==0);
        CHECK(n==m && !memcmp(a,b,n)); pcm_dsd_destroy(simd); pcm_dsd_destroy(scalar);
    }
    puts("PASS: SIMD/scalar byte identity, state identity, independent RNG, overload, carry, reset, mono/stereo, filters and modes");
#else
    puts("SIMD disabled: scalar build");
#endif
    return 0;
}
