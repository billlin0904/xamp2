/* DSP option regression: streaming contracts and independent signal checks. */
#define PCM_DSD_BUILD
#include "../src/pcm_dsd_converter.c"
#include <stdio.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static int collect(pcm_dsd_converter* c,const double* pcm,size_t frames,size_t chunk,
    uint8_t* result,size_t* total) {
    size_t pos=0,used,written; uint8_t output[514]; int status;
    *total=0;
    while(pos<frames) {
        size_t count=frames-pos;if(count>chunk) count=chunk;
        status=pcm_dsd_process(c,pcm+pos*2,count,output,sizeof(output),&used,&written);
        CHECK(status==0 && (used || written));
        pos+=used;memcpy(result+*total,output,written);*total+=written;
    }
    do {
        status=pcm_dsd_flush(c,output,sizeof(output),&written);
        CHECK(status==0 || status==PCM_DSD_FINISHED);
        memcpy(result+*total,output,written);*total+=written;
    } while(status!=PCM_DSD_FINISHED);
    return 0;
}
int main(void) {
    pcm_dsd_config config;pcm_dsd_options o;pcm_dsd_converter* c=NULL;
    double pcm[8194];uint8_t first[16388],second[16388];size_t i,n,m;
    uint32_t f,mod,b;
    pcm_dsd_default_config(&config);pcm_dsd_default_options(&o);config.dsd_multiplier=16;
    for(i=0;i<8194;i++) pcm[i]=0.8*sin((double)i*0.031);
    for(f=0;f<3;f++) for(mod=0;mod<2;mod++) for(b=64;b<=1024;b*=2) {
        o.filter=f;o.modulator=mod;o.block_frames=b;
        CHECK(pcm_dsd_create_ex(&config,&o,&c)==0);
        CHECK(collect(c,pcm,4097,4097,first,&n)==0 && n==16388);
        CHECK(pcm_dsd_reset(c)==0);
        CHECK(collect(c,pcm,4097,13,second,&m)==0 && m==n && !memcmp(first,second,n));
        pcm_dsd_destroy(c);
    }
    pcm_dsd_default_options(&o);
    CHECK(pcm_dsd_create(&config,&c)==0);CHECK(collect(c,pcm,4097,37,first,&n)==0);pcm_dsd_destroy(c);
    CHECK(pcm_dsd_create_ex(&config,&o,&c)==0);CHECK(collect(c,pcm,4097,37,second,&m)==0);
    CHECK(n==m && !memcmp(first,second,n));pcm_dsd_destroy(c);
    o.block_frames=255;CHECK(pcm_dsd_create_ex(&config,&o,&c)==PCM_DSD_INVALID_ARGUMENT && !c);
    o.block_frames=2048;CHECK(pcm_dsd_create_ex(&config,&o,&c)==PCM_DSD_INVALID_ARGUMENT && !c);
    pcm_dsd_default_options(&o);o.filter=3;CHECK(pcm_dsd_create_ex(&config,&o,&c)==PCM_DSD_INVALID_ARGUMENT && !c);
    o.filter=0;o.modulator=2;CHECK(pcm_dsd_create_ex(&config,&o,&c)==PCM_DSD_INVALID_ARGUMENT && !c);
    /* Recover FIR taps from the actual FFT plan, check symmetry, pass/stop bands,
       then compare overlap-add output with independent time-domain convolution. */
    for(f=0;f<3;f++) for(b=64;b<=1024;b*=2) {
        double h[TAPS],input[8192],worst=0.0,dc=0.0,pass=0.0,stop=0.0;
        size_t offset,j,k;fir_stage* st;
        pcm_dsd_default_options(&o);o.filter=f;o.block_frames=b;
        config.input_sample_rate=352800; /* 2x to DSD16 */
        CHECK(pcm_dsd_create_ex(&config,&o,&c)==0);st=&c->stage[0];
        memcpy(c->scratch,st->filter,st->nfft*sizeof(complex64));fft(c->scratch,st->nfft,1);
        for(i=0;i<c->taps;i++) h[i]=c->scratch[i].re;
        for(i=0;i<c->taps;i++) {
            const double x=(double)i-(double)(c->taps-1)/2.0;
            CHECK(fabs(h[i]-h[c->taps-1-i])<1e-10);
            dc+=h[i];pass+=h[i]*cos(0.45*3.141592653589793*x);
            stop+=h[i]*cos(0.55*3.141592653589793*x);
        }
        CHECK(fabs(dc-2.0)<1e-8 && fabs(pass-2.0)<0.001 && fabs(stop)<0.001);
        for(i=0;i<8192;i++) input[i]=sin((double)i*0.17);
        for(offset=0;offset<4096;offset+=b) {
            for(i=0;i<b;i++) { c->input[i*2]=input[offset+i];c->input[i*2+1]=-input[offset+i]*0.4; }
            convert_block(c,b);
            for(i=0;i<b*2;i++) {
                double expected=0.0;const size_t t=offset*2+i;
                for(j=0;j<c->taps && j<=t;j++) if((t-j)%2==0) { k=(t-j)/2;expected+=input[k]*h[j]; }
                if(fabs(c->b[i*2]-expected)>worst) worst=fabs(c->b[i*2]-expected);
                CHECK(fabs(c->b[i*2+1]+expected*0.4)<1e-10);
            }
        }
        CHECK(worst<1e-10);pcm_dsd_destroy(c);
    }
    /* DC density and bounded integrator under silence, DC and overload. */
    for(int level=-12;level<=12;level++) {
        mod_state state={0};double mean=0.0,target=(double)level/10.0;
        for(i=0;i<100000;i++) {
            mean+=modulate_first(target,1.0,0.0,&state)?1.0:-1.0;
            CHECK(isfinite(state.feedback) && fabs(state.feedback)<=1.000000001);
        }
        if(target>1) target=1;if(target< -1) target=-1;
        CHECK(fabs(mean/100000.0-target)<0.00003);
    }
    puts("PASS: 30 DSP combinations, chunk invariance, reset/EOF, ABI defaults, FIR response/convolution, first-order DC/overload");
    return 0;
}
