#ifndef __UK_CET__
#define __UK_CET__

#define SHSTK_SIZE			(512 * PAGE_SIZE)

int ukcet_cpu_supports_ibt();
int ukcet_cpu_supports_shadow_stack();
void* ukcet_create_shstk();
void* ukcet_create_isst();
void ukcet_unmap_isst(void *isst);

#endif
