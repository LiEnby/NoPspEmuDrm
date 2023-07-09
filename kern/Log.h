#include "../Config.h"
#ifndef PSP_LOG_H
#define PSP_LOG_H 1

#ifdef LOGGING_ENABLED
#define log(...) ksceKernelPrintf(__VA_ARGS__ )
#else
#define log(...) /* nothing */
#endif

#endif