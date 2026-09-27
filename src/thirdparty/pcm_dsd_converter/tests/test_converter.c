#include "pcm_dsd_converter.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
int main(void) {
    pcm_dsd_config config;
    pcm_dsd_converter* c=NULL;
    double pcm[34]={0}; uint8_t output[512], first[512];
    size_t used,written,total=0; int result;
    pcm_dsd_default_config(&config);
    config.dsd_multiplier=16;
    CHECK(pcm_dsd_create(&config,&c)==PCM_DSD_OK);
    CHECK(pcm_dsd_output_sample_rate(c)==705600);
    CHECK(pcm_dsd_process(c,pcm,17,output,sizeof(output),&used,&written)==0);
    CHECK(used==17 && written==0);
    do { result=pcm_dsd_flush(c,output+total,sizeof(output)-total,&written); total+=written; } while(result==0);
    CHECK(result==PCM_DSD_FINISHED && total==68);
    memcpy(first,output,total);
    CHECK(pcm_dsd_process(c,pcm,1,output,sizeof(output),&used,&written)==PCM_DSD_INVALID_STATE);
    CHECK(pcm_dsd_reset(c)==0);
    CHECK(pcm_dsd_process(c,pcm,17,output,sizeof(output),&used,&written)==0);
    CHECK(pcm_dsd_flush(c,output,sizeof(output),&written)==PCM_DSD_FINISHED);
    CHECK(written==total && memcmp(first,output,total)==0);
    pcm_dsd_destroy(c);
    config.input_sample_rate=12345;
    CHECK(pcm_dsd_create(&config,&c)==PCM_DSD_INVALID_ARGUMENT && c==NULL);
    puts("C ABI smoke test passed"); return 0;
}
