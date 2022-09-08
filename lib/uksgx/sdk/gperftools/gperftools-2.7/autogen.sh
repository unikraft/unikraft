#!/bin/sh

srcdir=`dirname $0`
[ -z "$srcdir" ] && srcdir=.

ORIGDIR=`pwd`
cd $srcdir

autoreconf -i

if [ "$1" = "DEBUG" ] 
then
    COMMON_FLAGS="-DTCMALLOC_SGX_DEBUG"
else
    COMMON_FLAGS="-D_FORTIFY_SOURCE=2"
fi

COMMON_FLAGS="$COMMON_FLAGS -DNO_HEAP_CHECK -DTCMALLOC_SGX -DTCMALLOC_NO_ALIASES"

CFLAGS="$CFLAGS $ENCLAVE_CFLAGS $COMMON_FLAGS"
CXXFLAGS="$CXXFLAGS $ENCLAVE_CXXFLAGS $COMMON_FLAGS"
CPPFLAGS="-I../../../common/inc -I../../../common/inc/tlibc -I../../../common/inc/internal/ -I../../../sdk/tlibcxx/include -I../../../sdk/trts/"
#if echo $CFLAGS | grep -q -- '-m32'; then
   HOST_OPT='--host=i386-linux-gnu'
#fi
export CFLAGS
export CXXFLAGS
export CPPFLAGS
 
#Insert following codes into configure after add "-mfunction-return=thunk-extern -mindirect-branch-register" option, Or the "checking whether the C compiler works..." check will fail
#  #pragma GCC push_options
#  #pragma GCC optimize ("-fomit-frame-pointer")
#  void __x86_return_thunk()
#  {
#      __asm__("ret\n\t");
#  }
#  void __x86_indirect_thunk_rax()
#  {
#      __asm__("jmp *%rax\n\t");
#  }
#  #pragma GCC pop_options 
line=`grep -n "__x86_return_thunk()" ./configure | cut -d: -f 1`
if [ -n "$line" ]; then
  echo "__x86_return_thunk() already exist..."
else
  line_end=`grep -n "\"checking whether the C compiler works... \"" ./configure | cut -d: -f 1`
  line_start=`expr $line_end - 30`  #Search an scope
  sed -i "${line_start},${line_end} s/^_ACEOF/#pragma GCC push_options\r\n#pragma GCC optimize (\"-fomit-frame-pointer\")\r\nvoid __x86_return_thunk(){__asm__(\"ret\\\n\\\t\");}\r\nvoid __x86_indirect_thunk_rax(){__asm__(\"jmp \*%rax\\\n\\\t\");}\r\n#pragma GCC pop_options\r\n_ACEOF/" ./configure
fi

$srcdir/configure $HOST_OPT --enable-shared=no \
   --with-pic \
   --disable-cpu-profiler \
   --disable-heap-profiler       \
   --disable-heap-checker \
   --disable-debugalloc \
   --enable-minimal

#must remove this attribute define in generated config.h, or can't debug tcmalloc with sgx-gdb
if [ "$1" = "DEBUG" ]
then
    sed -i 's/#define HAVE___ATTRIBUTE__ 1/\/\/#define HAVE___ATTRIBUTE__ 1/g' src/config.h
fi

#must remove "HAVE_SYS_SYSCALL_H" "HAVE_MMAP" "HAVE_PROGRAM_INVOCATION_NAME"
sed -i 's/#define HAVE_SYS_SYSCALL_H/\/\/#define HAVE_SYS_SYSCALL_H/g' src/config.h
sed -i 's/#define HAVE_MMAP/\/\/#define HAVE_MMAP/g' src/config.h
sed -i 's/#define HAVE_PROGRAM_INVOCATION_NAME/\/\/#define HAVE_PROGRAM_INVOCATION_NAME/g' src/config.h

#Insert SGX special codes into config.h
################################
#  #ifdef TCMALLOC_SGX
#  #include <sgx_trts.h>
#  #include <sgx_thread.h>
#  #include <sgx_spinlock.h>
#  enum {PTHREAD_ONCE_INIT = 0};
#  #define pthread_once_t long
#  #define pthread_key_t sgx_thread_t
#  #define pthread_t sgx_thread_t
#  #define pthread_self sgx_thread_self
#  /*SGX special, implemented in sgx_utils.cc*/
#  extern "C" char *getenv(const char *name);
#  extern "C" int GetSystemCPUsCount();
#  enum { STDIN_FILENO = 0, STDOUT_FILENO = 1, STDERR_FILENO = 2 };
#  extern "C" size_t write(int fd, const void *buf, size_t count);
#  extern "C" int getpagesize();  //it didn't delare include enclave's <unistd.h>
#  #endif
################################
sed -i '/#endif  \/\* #ifndef GPERFTOOLS_CONFIG_H_ \*\//i#ifdef TCMALLOC_SGX' src/config.h
sed -i '/#ifdef TCMALLOC_SGX/a#include <sgx_trts.h>' src/config.h
sed -i '/#include <sgx_trts.h>/a#include <sgx_thread.h>' src/config.h
sed -i '/#include <sgx_thread.h>/a#include <sgx_spinlock.h>' src/config.h
sed -i '/#include <sgx_spinlock.h>/aenum {PTHREAD_ONCE_INIT = 0};' src/config.h
sed -i '/enum {PTHREAD_ONCE_INIT = 0};/a#define pthread_once_t long' src/config.h
sed -i '/#define pthread_once_t long/a#define pthread_key_t sgx_thread_t' src/config.h
sed -i '/#define pthread_key_t sgx_thread_t/a#define pthread_t sgx_thread_t' src/config.h
sed -i '/#define pthread_t sgx_thread_t/a#define pthread_self sgx_thread_self' src/config.h
sed -i '/#define pthread_self sgx_thread_self/a\/\*SGX special, implemented in sgx_utils.cc\*\/' src/config.h
sed -i '/\/\*SGX special, implemented in sgx_utils.cc\*\//aextern "C" char \*getenv(const char \*name);' src/config.h
sed -i '/extern "C" char \*getenv(const char \*name);/aextern "C" int GetSystemCPUsCount();' src/config.h
sed -i '/extern "C" int GetSystemCPUsCount();/aenum { STDIN_FILENO = 0, STDOUT_FILENO = 1, STDERR_FILENO = 2 };' src/config.h
sed -i '/enum { STDIN_FILENO = 0, STDOUT_FILENO = 1, STDERR_FILENO = 2 };/aextern "C" size_t write(int fd, const void \*buf, size_t count);' src/config.h
sed -i '/extern "C" size_t write(int fd, const void \*buf, size_t count);/aextern "C" int getpagesize\(\);' src/config.h
sed -i '/extern "C" int getpagesize();/a#endif  \/\* #ifdef TCMALLOC_SGX \*\/' src/config.h


