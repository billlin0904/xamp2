/* Run manually in Release; excludes descriptor creation and warms the DSP path. */
#define PCM_DSD_BUILD
#include "../src/pcm_dsd_converter.c"
#include <stdio.h>
#include <time.h>
int main(void) {
    unsigned rate, repeat;
    for(rate=64;rate<=256;rate*=2) {
        pcm_dsd_config cfg; pcm_dsd_converter* c=NULL;
        pcm_dsd_default_config(&cfg); cfg.dsd_multiplier=rate;
        if(pcm_dsd_create(&cfg,&c)!=PCM_DSD_OK) return 1;
        for(repeat=0;repeat<4;repeat++) {
            size_t pos,i; clock_t start; double seconds;
            pcm_dsd_reset(c); start=clock();
            for(pos=0;pos<44100;pos+=BLOCK) {
                const size_t count=44100-pos<BLOCK?44100-pos:BLOCK;
                for(i=0;i<count;i++) {
                    c->input[i*2]=0.5*sin((double)(pos+i)*0.031);
                    c->input[i*2+1]=0.4*cos((double)(pos+i)*0.073);
                }
                if(!convert_block(c,count)) return 1;
            }
            seconds=(double)(clock()-start)/CLOCKS_PER_SEC;
            if(repeat) printf("DSD%u run%u %.6f seconds %.3fx realtime\n",rate,repeat,seconds,1.0/seconds);
        }
        pcm_dsd_destroy(c);
    }
    return 0;
}
