// LibALAC.h - Contains declarations of LibALAC functions
#pragma once

#ifdef LIBALAC_EXPORTS  
#define LIBALAC_API __declspec(dllexport)   
#else  
#define LIBALAC_API __declspec(dllimport)   
#endif  

// Encoder-Constructor 
extern "C" LIBALAC_API void* InitializeEncoder(int sampleRate, int channels, int bitsPerSample, int framesPerPacket, bool useFastMode);

// Get size of magic cookie
extern "C" LIBALAC_API int GetMagicCookieSize(void* encoder);

// Get the magic cookie
extern "C" LIBALAC_API int GetMagicCookie(void* encoder, unsigned char * outCookie);

// Encode the next block of samples
extern "C" LIBALAC_API int Encode(void* encoder, unsigned char * inBuffer, unsigned char * outBuffer, int * ioNumBytes);

// Drain out any leftover samples and free memory
extern "C" LIBALAC_API int FinishEncoder(void* encoder);

// Decoder-Constructor 
extern "C" LIBALAC_API void* InitializeDecoder(int sampleRate, int channels, int bitsPerSample, int framesPerPacket);

// Decoder-Constructor 
extern "C" LIBALAC_API void* InitializeDecoderWithCookie(void * inMagicCookie, int inMagicCookieSize);

// Decode the next block of samples
extern "C" LIBALAC_API int Decode(void* decoder, unsigned char * inBuffer, unsigned char * outBuffer, int * ioNumBytes);

// Free the memory
extern "C" LIBALAC_API int FinishDecoder(void* decoder);

// Get info from magic cookie
extern "C" LIBALAC_API int ParseMagicCookie(void * inMagicCookie, int inMagicCookieSize, int * outSampleRate, int * outChannels, int * outBitsPerSample, int * outFramesPerPacket);
