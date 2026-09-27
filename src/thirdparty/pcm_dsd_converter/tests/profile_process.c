/* Windows QPC section profiling; production DLL has no timers or counters. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
enum { block_copy, zero_insert, forward_fft, multiply, inverse_fft, overlap,
       output_copy, tail_copy, modulate_pack, drain_copy, input_copy, section_count };
static const char* names[] = {"block_copy","zero_insert","forward_fft","multiply",
    "inverse_fft","overlap","output_copy","tail_copy","modulate_pack","drain_copy","input_copy"};
static int64_t ticks[section_count];
static int64_t now(void) { LARGE_INTEGER value; QueryPerformanceCounter(&value); return value.QuadPart; }
#ifdef PCM_DSD_ENABLE_PROFILE
#define PCM_DSD_PROFILE_BEGIN() int64_t profile_start=now()
#define PCM_DSD_PROFILE_END(section) do { int64_t end=now(); ticks[section]+=end-profile_start; profile_start=end; } while(0)
#endif
#define PCM_DSD_BUILD
#include "../src/pcm_dsd_converter.c"
int main(int argc, char** argv) {
    LARGE_INTEGER frequency;
    unsigned rate,block,run;
    const size_t frames=44100;
    double* input=(double*)malloc(frames*2*sizeof(double));
    uint8_t output[65536]; size_t i;
    /* Keep both harnesses on the same logical CPU on hybrid-core machines. */
    if(!SetThreadAffinityMask(GetCurrentThread(),(DWORD_PTR)1)) return 6;
    QueryPerformanceFrequency(&frequency);
    if(!input) return 1;
    for(i=0;i<frames;i++) {
        input[i*2]=0.5*sin(6.283185307179586*440*i/44100.0);
        input[i*2+1]=0.4*cos(6.283185307179586*997*i/44100.0);
    }
    puts("rate,block,run,process_ms,flush_ms,bytes,hash,section,section_ms");
    for(block=256;block<=1024;block*=4) for(rate=64;rate<=256;rate*=2) {
        pcm_dsd_config cfg; pcm_dsd_options opts; pcm_dsd_converter* c=NULL;
        pcm_dsd_default_config(&cfg); pcm_dsd_default_options(&opts);
        cfg.dsd_multiplier=rate; opts.block_frames=block;
        if(pcm_dsd_create_ex(&cfg,&opts,&c)!=PCM_DSD_OK) return 2;
        if(argc==2 && strcmp(argv[1],"--scalar")==0) {
            c->stereo_simd=0; c->multiply=multiply_scalar;
        }
        for(run=0;run<6;run++) {
            size_t pos=0,total=0,used,written,j; int status;
            int64_t elapsed=0,flush_time,start,saved[section_count];
            uint64_t hash=UINT64_C(14695981039346656037);
            pcm_dsd_reset(c); memset(ticks,0,sizeof(ticks));
            while(pos<frames) {
                size_t count=frames-pos; if(count>1024) count=1024;
                start=now();
                status=pcm_dsd_process(c,input+pos*2,count,output,sizeof(output),&used,&written);
                elapsed+=now()-start;
                if(status!=PCM_DSD_OK || (!used && !written)) return 3;
                pos+=used; total+=written;
                for(j=0;j<written;j++) hash=(hash^output[j])*UINT64_C(1099511628211);
            }
            memcpy(saved,ticks,sizeof(saved)); flush_time=0;
            do {
                start=now(); status=pcm_dsd_flush(c,output,sizeof(output),&written); flush_time+=now()-start;
                if(status!=PCM_DSD_OK && status!=PCM_DSD_FINISHED) return 4;
                total+=written;
                for(j=0;j<written;j++) hash=(hash^output[j])*UINT64_C(1099511628211);
            } while(status!=PCM_DSD_FINISHED);
            if(total!=frames*rate/4) return 5;
            if(run) for(i=0;i<section_count;i++) printf("%u,%u,%u,%.6f,%.6f,%zu,%016llx,%s,%.6f\n",
                rate,block,run,elapsed*1000.0/frequency.QuadPart,flush_time*1000.0/frequency.QuadPart,
                total,(unsigned long long)hash,names[i],saved[i]*1000.0/frequency.QuadPart);
        }
        pcm_dsd_destroy(c);
    }
    free(input); return 0;
}
