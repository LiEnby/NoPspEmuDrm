#ifndef PSP_LOG_H
#define PSP_LOG_H 1

#ifdef LOGGING_ENABLED
#define log(...) sceClibPrintf(__VA_ARGS__ )
#else
#define log(...) /* nothing */
#endif

#ifdef LOGGING_ENABLED
#define logBuf(buf,sz) for(int i = 0; i < sz ; i++) { sceClibPrintf("%x ", buf[i]); }; sceClibPrintf("\n");
#else
#define logBuf(buf,sz) /* nothing */
#endif

#endif