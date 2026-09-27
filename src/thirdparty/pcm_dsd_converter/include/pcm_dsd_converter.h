#ifndef PCM_DSD_CONVERTER_H
#define PCM_DSD_CONVERTER_H
#include <stddef.h>
#include <stdint.h>
#if defined(_WIN32)
# if defined(PCM_DSD_BUILD)
#  define PCM_DSD_API __declspec(dllexport)
# else
#  define PCM_DSD_API __declspec(dllimport)
# endif
# define PCM_DSD_CALL __cdecl
#else
# define PCM_DSD_API __attribute__((visibility("default")))
# define PCM_DSD_CALL
#endif
#ifdef __cplusplus
extern "C" {
#endif
typedef struct pcm_dsd_converter pcm_dsd_converter;
enum { PCM_DSD_OK=0, PCM_DSD_INVALID_ARGUMENT=1, PCM_DSD_OUT_OF_MEMORY=2,
       PCM_DSD_FINISHED=3, PCM_DSD_INVALID_STATE=4 };
typedef struct pcm_dsd_config {
    uint32_t struct_size;
    uint32_t input_sample_rate;
    uint32_t channels; /* 1 or 2; mono is duplicated to stereo */
    uint32_t dsd_multiplier; /* 16..2048, power of two */
    double input_gain; /* linear, default 0.5 (includes modulator headroom) */
    double dither_amplitude; /* default 2^-24, 0 disables */
    uint64_t seed;
} pcm_dsd_config;
/* Optional DSP choices. Separate structure keeps the original C ABI intact. */
enum { PCM_DSD_FIR_ORIGINAL=0, PCM_DSD_FIR_BLACKMAN_1023=1, PCM_DSD_FIR_BLACKMAN_511=2 };
enum { PCM_DSD_MODULATOR_15=0, PCM_DSD_MODULATOR_1=1 };
typedef struct pcm_dsd_options {
    uint32_t struct_size;
    uint32_t filter;
    uint32_t modulator;
    uint32_t block_frames; /* 64, 128, 256 (default), 512, 1024 */
} pcm_dsd_options;
PCM_DSD_API void PCM_DSD_CALL pcm_dsd_default_options(pcm_dsd_options* options);
PCM_DSD_API int PCM_DSD_CALL pcm_dsd_create_ex(const pcm_dsd_config* config,
    const pcm_dsd_options* options, pcm_dsd_converter** out);
PCM_DSD_API void PCM_DSD_CALL pcm_dsd_default_config(pcm_dsd_config* config);
PCM_DSD_API int PCM_DSD_CALL pcm_dsd_create(const pcm_dsd_config* config, pcm_dsd_converter** out);
/* Input: normalized interleaved doubles. Output: MSB-first packed DSD,
   byte-interleaved L,R,L,R. Capacity and written are BYTES, consumed is PCM FRAMES.
   May consume/produce partially or consume without output. Capacity must be >=2.
   Buffers must not overlap. No allocation in process/flush. */
PCM_DSD_API int PCM_DSD_CALL pcm_dsd_process(pcm_dsd_converter* converter,
    const double* pcm, size_t frames, uint8_t* output, size_t capacity,
    size_t* consumed, size_t* written);
/* Call repeatedly until FINISHED, keeping bytes from every call (including FINISHED).
   Truncates FIR tail to original duration, pads final DSD byte with zero bits.
   process is forbidden after flush until reset. */
PCM_DSD_API int PCM_DSD_CALL pcm_dsd_flush(pcm_dsd_converter* converter,
    uint8_t* output, size_t capacity, size_t* written);
PCM_DSD_API int PCM_DSD_CALL pcm_dsd_reset(pcm_dsd_converter* converter);
PCM_DSD_API void PCM_DSD_CALL pcm_dsd_destroy(pcm_dsd_converter* converter);
PCM_DSD_API uint32_t PCM_DSD_CALL pcm_dsd_output_sample_rate(const pcm_dsd_converter* converter);
PCM_DSD_API const char* PCM_DSD_CALL pcm_dsd_status_string(int status);
/* Compatibility ABI expected by dsd15_core.py. One already-upsampled mono channel.
   sos: 8x6 doubles; states: 8x2 doubles. All state pointers are in/out.
   carry_count 0..7; output consists only of complete bytes. */
PCM_DSD_API int PCM_DSD_CALL dsd15_modulate_pack(const double* samples, int64_t frames,
    const double* sos, double gain, double dither, uint64_t* rng, double* states,
    double* feedback, uint8_t* previous, int64_t* run, double* last_return,
    int32_t* carry_byte, int32_t* carry_count, uint8_t* output,
    int64_t capacity, int64_t* written);
#ifdef __cplusplus
}
#endif
#endif
