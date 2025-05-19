#ifndef __UK_CET__
#define __UK_CET__

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#define SHSTK_SIZE			(512 * PAGE_SIZE)

#define SHSTK_BASE(shstk) ((unsigned long long)(((char*)(shstk)) - PAGE_SIZE - 8 + SHSTK_SIZE))

int ukcet_cpu_supports_ibt();
int ukcet_cpu_supports_shadow_stack();
void* ukcet_create_shstk();
void* ukcet_create_isst();
void ukcet_unmap_isst(void *isst);

#endif
