#ifndef __UKARCH_X86_CET__
#define __UKARCH_X86_CET__

#if __ASSEMBLY__
    #if (CONFIG_X86_64_CET_IBT && (__CET__ & 1))
        #define X86_CET_ENDBR endbr64
        #define X86_CET_ENDBR_NOTRACK notrack
    #else
        #define X86_CET_ENDBR
        #define X86_CET_ENDBR_NOTRACK
    #endif
#endif

#endif
