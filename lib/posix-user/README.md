# posix-user: Unikraft's POSIX User Identity

The `posix-user` library includes structures and functions that implement the concepts of users and groups in `Unikraft`.

## Key Data Structures

### passwd Structure

```c
static struct passwd pw__ = {
 .pw_name = UK_DEFAULT_USER,
 .pw_passwd = UK_DEFAULT_PASS,
 .pw_uid = UK_DEFAULT_UID,
 .pw_gid = UK_DEFAULT_GID,
 .pw_gecos = UK_DEFAULT_USER,
 .pw_dir = "/",
 .pw_shell = "",
};
```

The `passwd` structure is used to define the default `passwd` entry, whose format is similar to the ones from the `etc/passwd` file in Linux:

* `pw_name` refers to the name of the user, `UK_DEFAULT_USER`, which is actually `CONFIG_LIBPOSIX_USER_USERNAME`.
* `pw_passwd` is the password of the default user, which is the empty string (`""`), meaning that the user does not have a password.
* `pw_uid` is the user ID of the default user, which is equal to `CONFIG_LIBPOSIX_USER_UID`.
* `pw_gid` is the ID of the default user's group and is equal to
`CONFIG_LIBPOSIX_USER_GID`.
* `pw_gecos` is used to record general information about the account or its user, such as their real name and phone number.
In this case, the `gecos` field only contains the name of the user, so it is equal to `pw_name`.
* `pw_dir` is the user home directory; in this case, it is `"/"`.
* `pw_shell` is the login shell of the default user.
This is equal to `""` because the user doesn't have a login shell.

### group Structure

```c
static struct group g__ = {
 .gr_name = UK_DEFAULT_GROUP,
 .gr_passwd = UK_DEFAULT_PASS,
 .gr_gid = UK_DEFAULT_GID,
 .gr_mem = g_members__,
};
```

The `group` structure is used to define the default `group` entry, whose format is similar to the ones from the `etc/group` file in Linux:

* `gr_name` refers to the name of the group, `UK_DEFAULT_GROUP`, which is actually `CONFIG_LIBPOSIX_USER_GROUPNAME`.
* `gr_passwd` is the password of the default group, which is the empty string (`""`), meaning that the group does not have a password.
* `gr_gid` is the ID of the default group and is equal to
`CONFIG_LIBPOSIX_USER_GID`.
* `gr_mem` is a list of usernames separated by commas.
These are the users who are members of the group.
In this case, we have a single member, the `UK_DEFAULT_USER`.
