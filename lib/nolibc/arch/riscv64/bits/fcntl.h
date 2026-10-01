#define O_CREAT        0100
#define O_EXCL         0200
#define O_TRUNC       01000
#define O_APPEND      02000
#define O_NONBLOCK    04000
#define O_DIRECTORY 0200000
#define O_CLOEXEC  02000000

#define O_PATH    010000000
#define O_NDELAY O_NONBLOCK

#define F_GETFD  1
#define F_SETFD  2
#define F_GETFL  3
