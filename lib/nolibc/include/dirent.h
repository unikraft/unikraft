#ifndef	_DIRENT_H
#define	_DIRENT_H

#ifdef __cplusplus
extern "C" {
#endif

#define __NEED_ino_t
#define __NEED_off_t
#if defined(_BSD_SOURCE) || defined(_GNU_SOURCE)
#define __NEED_size_t
#endif

#include <nolibc-internal/shareddefs.h>

typedef struct __dirstream DIR;

#define _DIRENT_HAVE_D_RECLEN
#define _DIRENT_HAVE_D_OFF
#define _DIRENT_HAVE_D_TYPE

/**
 * The dirent and dirent64 structures must match the Linux system call ABI
 * for binary compatibility.
 */
struct dirent64 {
	ino_t d_ino;
	off_t d_off;
	unsigned short d_reclen;
	unsigned char d_type;
	char d_name[256];
};

#if CONFIG_LIBVFSCORE_NONLARGEFILE
struct dirent {
	unsigned long d_ino;
	unsigned long d_off;
	unsigned short d_reclen;
	char d_name[256];
	unsigned char pad;
	unsigned char d_type;
};
#else /* !CONFIG_LIBVFSCORE_NONLARGEFILE */
#define dirent dirent64
#endif /* !CONFIG_LIBVFSCORE_NONLARGEFILE */

#define d_fileno d_ino

int            closedir(DIR *);
DIR           *fdopendir(int);
DIR           *opendir(const char *);
struct dirent64 *readdir64(DIR *dir);
int            readdir64_r(DIR *__restrict, struct dirent64 *__restrict,
		struct dirent64 **__restrict);
struct dirent *readdir(DIR *);
int            readdir_r(DIR *__restrict, struct dirent *__restrict, struct dirent **__restrict);

void           rewinddir(DIR *);
int            dirfd(DIR *);

int alphasort(const struct dirent **, const struct dirent **);
int scandir(const char *, struct dirent ***, int (*)(const struct dirent *), int (*)(const struct dirent **, const struct dirent **));

#if defined(_XOPEN_SOURCE) || defined(_GNU_SOURCE) || defined(_BSD_SOURCE)
void           seekdir(DIR *, long);
long           telldir(DIR *);
#endif

#if defined(_GNU_SOURCE) || defined(_BSD_SOURCE)
#define DT_UNKNOWN 0
#define DT_FIFO 1
#define DT_CHR 2
#define DT_DIR 4
#define DT_BLK 6
#define DT_REG 8
#define DT_LNK 10
#define DT_SOCK 12
#define DT_WHT 14
#define IFTODT(x) ((x)>>12 & 017)
#define DTTOIF(x) ((x)<<12)
int getdents(int fd, struct dirent *dirp, size_t count);
int getdents64(int fd, struct dirent64 *dirp, size_t count);
#endif

#ifdef _GNU_SOURCE
int versionsort(const struct dirent64 **, const struct dirent64 **);
#endif

#if defined(_LARGEFILE64_SOURCE) || defined(_GNU_SOURCE)

/**
 * `alphasort()` and `versionsort()` are not provided in the Unikraft core,
 * so we can just leave the largefile functions as aliases to the non largefile
 * definitions
 */
#define alphasort64 alphasort
#define versionsort64 versionsort
#define off64_t off_t
#define ino64_t ino_t
#endif

#ifdef __cplusplus
}
#endif

#endif
