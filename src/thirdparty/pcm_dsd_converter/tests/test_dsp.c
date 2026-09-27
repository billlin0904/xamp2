/* White-box numerical verification against independent direct FIR convolution. */
#define _CRT_SECURE_NO_WARNINGS
#define PCM_DSD_BUILD
#include "../src/pcm_dsd_converter.c"
#include <stdio.h>
int main(int argc, char** argv) {
    size_t i,j; double worst=0.0;
    complex64* a=(complex64*)calloc(8192,sizeof(complex64));
    complex64* h=(complex64*)calloc(8192,sizeof(complex64));
    if(!a || !h) return 1;
    for(i=0;i<512;i+=2) a[i].re=sin((double)i*0.37);
    for(i=0;i<TAPS;i++) h[i].re=fir[i]*2;
    fft(a,8192,0); fft(h,8192,0);
    for(i=0;i<8192;i++) { complex64 v=a[i]; a[i].re=v.re*h[i].re-v.im*h[i].im; a[i].im=v.re*h[i].im+v.im*h[i].re; }
    fft(a,8192,1);
    for(i=0;i<512+TAPS-1;i++) {
        double expected=0.0, error;
        for(j=0;j<512;j+=2) if(i>=j && i-j<TAPS) expected+=sin((double)j*0.37)*fir[i-j]*2;
        error=fabs(a[i].re-expected); if(error>worst) worst=error;
    }
    free(a); free(h);
    printf("FFT FIR maximum absolute error: %.17g\n",worst);
    if(worst>=1e-10) return 1;
    {
        pcm_dsd_config config; pcm_dsd_converter* c=NULL;
        size_t n; double error=0.0;
        pcm_dsd_default_config(&config);
        if(pcm_dsd_create(&config,&c)!=PCM_DSD_OK) return 1;
        n=c->stage[0].nfft;
        a=(complex64*)calloc(n,sizeof(complex64));
        h=(complex64*)calloc(n,sizeof(complex64));
        if(!a || !h) return 1;
        for(i=0;i<512;i++) { a[i].re=sin(i*0.037); a[i].im=cos(i*0.091); }
        memcpy(h,a,n*sizeof(complex64));
        fft(h,n,0);
        if(!planned_fft(a,&c->stage[0],0)) return 1;
        for(i=0;i<n;i++) {
            const double e=fmax(fabs(a[i].re-h[i].re),fabs(a[i].im-h[i].im));
            if(e>error) error=e;
        }
        if(error>1e-8 || !planned_fft(a,&c->stage[0],1)) return 1;
        for(i=0;i<n;i++) {
            const double re=i<512?sin(i*0.037):0.0, im=i<512?cos(i*0.091):0.0;
            if(fabs(a[i].re-re)>1e-10 || fabs(a[i].im-im)>1e-10) return 1;
        }
        printf("Backend FFT reference error: %.17g; complex round-trip passed\n",error);
        free(a); free(h); pcm_dsd_destroy(c);
    }
    if(argc==2) {
        pcm_dsd_config config; pcm_dsd_converter* c=NULL;
        size_t pos=0, total=9001;
        FILE* file=NULL;
        pcm_dsd_default_config(&config);
        if(pcm_dsd_create(&config,&c)!=0) return 1;
        file=fopen(argv[1],"wb");
        if(!file) { pcm_dsd_destroy(c); return 1; }
        while(pos<total) {
            size_t count=total-pos;
            double* upsampled;
            if(count>BLOCK) count=BLOCK;
            for(i=0;i<count;i++) {
                c->input[i*2]=0.8*sin((double)(pos+i)*0.031);
                c->input[i*2+1]=0.5*cos((double)(pos+i)*0.073);
            }
            convert_block(c,count);
            upsampled=(c->stages%2)?c->b:c->a;
            if(fwrite(upsampled,sizeof(double),count*c->up*2,file)!=count*c->up*2) {
                fclose(file); pcm_dsd_destroy(c); return 1;
            }
            pos+=count;
        }
        fclose(file); pcm_dsd_destroy(c);
    }
    return 0;
}
