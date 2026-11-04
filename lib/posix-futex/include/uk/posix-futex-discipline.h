/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef __UK_POSIX_FUTEX_DISCIPLINE_H__
#define __UK_POSIX_FUTEX_DISCIPLINE_H__

#define FUTEX_DISCIPLINE_FIFO  0
#define FUTEX_DISCIPLINE_PRIO  1

int uk_futex_set_discipline(void *addr, int discipline);
int uk_futex_del_discipline(void *addr);

#endif /* __UK_POSIX_FUTEX_DISCIPLINE_H__ */
