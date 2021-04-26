#ifndef __UK_MBOX_ISR_H__
#define __UK_MBOX_ISR_H__

#include <uk/mbox.h>

#if CONFIG_LIBUKMPI_MBOX

int uk_mbox_post_try_isr(struct uk_mbox *m, void *msg);
int uk_mbox_recv_try_isr(struct uk_mbox *m, void **msg);

#endif /* CONFIG_LIBUKMPI_MBOX */
#endif /* __UK_MBOX_ISR_H__ */
