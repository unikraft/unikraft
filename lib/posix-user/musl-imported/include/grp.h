#ifndef	_GRP_H
#define	_GRP_H

#ifdef __cplusplus
extern "C" {
#endif

#define __NEED_size_t
#define __NEED_gid_t

#ifdef _GNU_SOURCE
#define __NEED_FILE
#include <stdio.h>
#endif

#include <sys/types.h>

/**
 * An entry containing the necessary data for defining a group
 */
struct group {
	/* Group name */
	char *gr_name;
	/* Group password */
	char *gr_passwd;
	/* Group identifier */
	gid_t gr_gid;
	/* List of group members */
	char **gr_mem;
};

/**
 * Searches through the groups (there is just the default group at the moment)
 * for a specific group ID and returns a pointer to an object with the group
 * information.
 *
 * @param gid
 *   Group id to search by
 * @return
 *   Pointer to the group with group id `gid`.
 */
struct group  *getgrgid(gid_t);
/**
 * Searches through the groups (there is just the default group at the moment)
 * for a specific group name and returns a pointer to an object with the group
 * information.
 *
 * @param name
 *   Group name to search by
 * @return
 *   Pointer to the group with group name `name`.
 */
struct group  *getgrnam(const char *);

/**
 * Like `getgrgid`, but stores the retrieved `group`
 * structure in the space pointed to by `grp`.
 *
 * @param gid
 *   Group id to search by
 * @param grp
 *	 Group structure in which the group with the `gid` group id is stored
 * @param buf
 *   Buffer which holds the string fields pointed to by the members of the
 *   `group` structure
 * @param buflen
 *	 Length of buffer `buf`
 * @param result
 *   Pointer to a location in which a pointer to the updated `group` structure
 *   is stored. If an error occurs or if the requested entry cannot be found,
 *   a NULL pointer is stored in this location
 * @return
 *   0 on success, `ERANGE` if `buflen` is not large enough.
 */
int getgrgid_r(gid_t, struct group *, char *, size_t, struct group **);
/**
 * Like `getgrnam`, but stores the retrieved `group` structure
 * in the space pointed to by `grp`.
 *
 * @param name
 *   Group name to search by
 * @param grp
 *	 Group structure in which the group with the `name` group name is stored
 * @param buf
 *   Buffer which holds the string fields pointed to by the members of the
 *   `group` structure
 * @param buflen
 *	 Length of buffer `buf`
 * @param result
 *   Pointer to a location in which a pointer to the updated `group` structure
 *   is stored. If an error occurs or if the requested entry cannot be found,
 *   a NULL pointer is stored in this location
 * @return
 *   0 on success, `ERANGE` if `buflen` is not large enough.
 */
int getgrnam_r(const char *, struct group *, char *, size_t, struct group **);

#if defined(_XOPEN_SOURCE) || defined(_GNU_SOURCE) || defined(_BSD_SOURCE)
/**
 * The first time this function is called, it returns the first entry.
 * Afterward, it returns successive entries.
 * This function allows iterating over all the groups from the array.
 *
 * @return
 *	 The current group entry.
 */
struct group  *getgrent(void);
/**
 * This function is normally used to close the group database,
 * but since, in this case, this is just an array, it does nothing.
 */
void           endgrent(void);
/**
 * Sets the groups iterator at the beginning of the `groups` array.
 */
void           setgrent(void);
#endif

#ifdef _GNU_SOURCE
struct group  *fgetgrent(FILE *);
int putgrent(const struct group *, FILE *);
#endif

#if defined(_GNU_SOURCE) || defined(_BSD_SOURCE)
/**
 * Gets the list of groups that the given user belongs to.
 *
 * @param user
 *   User for whom to get the group list in which it belongs
 * @param group
 *   Group which will be included in `groups`,
 *   if it was not among the groups defined for `user`
 * @param groups
 *   Array of groups the user belongs to
 * @param ngroups
 *   Maximum number of groups to return in the list at input, number of groups
 *   the user belogs to at output
 * @return
 *	 The number of groups the user belongs to, or -1 on error.
 */
int getgrouplist(const char *, gid_t, gid_t *, int *);
/**
 * Sets the supplementary group IDs for the calling process. There is only one
 * group (the default group), so this function only tests if the caller tries
 * to set any invalid groups.
 *
 * @param size
 *   Size of the `list` buffer
 * @param list
 *   List of supplementary group IDs
 * @return
 *	 0 on success, -1 on error.
 */
int setgroups(size_t, const gid_t *);
int initgroups(const char *, gid_t);
#endif

#ifdef __cplusplus
}
#endif

#endif
