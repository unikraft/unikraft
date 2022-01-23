#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uk/swrand.h>


struct CPUIDinfo {
    unsigned int EAX;
    unsigned int EBX;
    unsigned int ECX;
    unsigned int EDX;
};

void cpuid_info(struct CPUIDinfo *info, const unsigned int func, const unsigned int subfunc) {
    __asm__ __volatile__ (
            "cpuid"
            : "=a"(info->EAX), "=b"(info->EBX), "=c"(info->ECX), "=d"(info->EDX)
            : "a"(func), "c"(subfunc)
    );
}

int has_amd_cpu() {
    struct CPUIDinfo info;
    cpuid_info(&info, 0, 0);

    return (memcmp((char *) (&info.EBX), "htuA", 4) == 0
            && memcmp((char *) (&info.EDX), "itne", 4) == 0
            && memcmp((char *) (&info.ECX), "DMAc", 4) == 0);
}


int has_intel_cpu() {
    struct CPUIDinfo info;
    cpuid_info(&info, 0, 0);

    return (memcmp((char *) (&info.EBX), "Genu", 4) == 0
            && memcmp((char *) (&info.EDX), "ineI", 4) == 0
            && memcmp((char *) (&info.ECX), "ntel", 4) == 0);
}

int has_RDRAND() {    
    if (!(has_amd_cpu() || has_intel_cpu()))
        return 0;

    struct CPUIDinfo info;
    cpuid_info(&info, 1, 0);

    static const unsigned int RDRAND_FLAG = (1 << 30);
    if ((info.ECX & RDRAND_FLAG) == RDRAND_FLAG)
        return 1;

    return 0;
}

ssize_t RDRAND_bytes(unsigned char* buf, size_t buflen)
{
    if (!has_RDRAND()) {
        return -1;
    }

    size_t idx = 0, rem = buflen;
    size_t safety = buflen / sizeof(unsigned int) + 4;

    unsigned int val;
    while (rem > 0 && safety > 0)
    {
        char rc;    
        __asm__ volatile(
                "rdrand %0 ; setc %1"
                : "=r" (val), "=qm" (rc)
        );

        if (rc)
        {
            size_t cnt = (rem < sizeof(val) ? rem : sizeof(val));
            memcpy(buf + idx, &val, cnt);

            rem -= cnt;
            idx += cnt;
        }
        else
        {
            safety--;
        }
    }

    *((volatile unsigned int*)&val) = 0;

    return (ssize_t)rem;
}