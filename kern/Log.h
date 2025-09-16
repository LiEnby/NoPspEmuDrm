#ifndef PSP_LOG_H
#define PSP_LOG_H 1

#ifdef LOGGING_ENABLED
#define LOG(...) ksceKernelPrintf(__VA_ARGS__ )
#else
#define LOG(...) /* nothing */
#endif


#ifdef LOGGING_ENABLED
#define logBuf(buf, sz) for(int i = 0; i < sz; i++) { ksceKernelPrintf("%x ", buf[i]); }; ksceKernelPrintf("\n");
#else
#define logBuf(buf, sz) /* nothing */
#endif

#endif