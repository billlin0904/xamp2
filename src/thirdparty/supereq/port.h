#pragma once

#ifdef _WIN32
#define SUPEREQ_NOVTABLE _declspec(novtable)
#ifdef SUPEREQ_API_EXPORTS
#define SUPEREQ_API __declspec(dllexport)
#else
#define SUPEREQ_API __declspec(dllimport)
#endif
#else
#define SUPEREQ_API
#define SUPEREQ_NOVTABLE
#endif
