#ifndef _VFSCORE_FS_H_
#define _VFSCORE_FS_H_

#include <fcntl.h>
/*
 * Kernel encoding of open mode; separate read and write bits that are
 * independently testable: 1 greater than the above.
 */
#define UK_FREAD           0x00000001
#define UK_FWRITE          0x00000002

#define UK_ALLPERMS (S_ISUID|S_ISGID|S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO)

static inline int vfscore_fflags(int oflags)
{
	int rw = oflags & O_ACCMODE;

	oflags &= ~O_ACCMODE;
	return (rw + 1) | oflags;
}

static inline int vfscore_oflags(int fflags)
{
	int rw = fflags & (UK_FREAD|UK_FWRITE);

	fflags &= ~(UK_FREAD|UK_FWRITE);
	return (rw - 1) | fflags;
}

#endif /* _VFSCORE_FS_H_ */
