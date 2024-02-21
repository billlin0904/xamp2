/*
	BASSenc_AAC 2.4 C/C++ header file
	Copyright (c) 2019 Un4seen Developments Ltd.

	See the BASSENC_AAC.CHM file for more detailed documentation
*/

#ifndef BASSENC_AAC_H
#define BASSENC_AAC_H

#include "bassenc.h"

#if BASSVERSION!=0x204
#error conflicting BASS and BASSenc_AAC versions
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BASSENCAACDEF
#define BASSENCAACDEF(f) WINAPI f
#endif

DWORD BASSENCAACDEF(BASS_Encode_AAC_GetVersion)();

HENCODE BASSENCAACDEF(BASS_Encode_AAC_Start)(DWORD handle, const char *options, DWORD flags, ENCODEPROCEX *proc, void *user);
HENCODE BASSENCAACDEF(BASS_Encode_AAC_StartFile)(DWORD handle, const char *options, DWORD flags, const char *filename);

#ifdef __cplusplus
}

#ifdef _WIN32
static inline HENCODE BASS_Encode_AAC_Start(DWORD handle, const WCHAR *options, DWORD flags, ENCODEPROCEX *proc, void *user)
{
	return BASS_Encode_AAC_Start(handle, (const char*)options, flags|BASS_UNICODE, proc, user);
}

static inline HENCODE BASS_Encode_AAC_StartFile(DWORD handle, const WCHAR *options, DWORD flags, const WCHAR *filename)
{
	return BASS_Encode_AAC_StartFile(handle, (const char*)options, flags|BASS_UNICODE, (const char*)filename);
}
#endif
#endif

#endif
