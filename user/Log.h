#ifndef PSP_LOG_H
#define PSP_LOG_H 1

//#define LOGGING_ENABLED 1

#ifdef LOGGING_ENABLED
#define log(...) sceClibPrintf(__VA_ARGS__ )
#else
#define log(...) /* nothing */
#endif

#endif