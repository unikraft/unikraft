#!/bin/sh

test -n "$srcdir" || srcdir=`dirname "$0"`
test -n "$srcdir" || srcdir=.
(
  cd "$srcdir" &&
  autoreconf --force -v --install
) || exit

CFLAGS="$CFLAGS -std=c99 -fno-builtin -DHAVE_SGX=1 -fPIC -DUNW_LOCAL_ONLY -fdebug-prefix-map=$(pwd)=/libunwind"
# Remove duplicated compiler options and filter out `-nostdinc'
CFLAGS=`echo $CFLAGS | tr ' ' '\n' | grep -v nostdinc | tr '\n' ' '`
export CFLAGS

#Insert following codes into configure after add "-mfunction-return=thunk-extern -mindirect-branch-register" option, Or the "checking whether the C compiler works..." check will fail
#Insert following codes into configure after add "-mfunction-return=thunk-extern -mindirect-branch-register" option, Or the "checking whether we are cross compiling... " check will fail
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

  line_end=`grep -n "\"checking whether we are cross compiling... \"" ./configure | cut -d: -f 1`
  line_start=`expr $line_end - 30`  #Search an scope
  sed -i "${line_start},${line_end} s/^_ACEOF/#pragma GCC push_options\r\n#pragma GCC optimize (\"-fomit-frame-pointer\")\r\nvoid __x86_return_thunk(){__asm__(\"ret\\\n\\\t\");}\r\nvoid __x86_indirect_thunk_rax(){__asm__(\"jmp \*%rax\\\n\\\t\");}\r\n#pragma GCC pop_options\r\n_ACEOF/" ./configure
fi

test -n "$NOCONFIGURE" || "$srcdir/configure" --enable-shared=no \
                                              --disable-block-signals \
                                              --enable-debug=no \
                                              --enable-debug-frame=no \
                                              --enable-coredump=no \
                                              --enable-ptrace=no \
                                              --enable-setjmp=no \
                                              --disable-tests    \
                                              --enable-cxx-exceptions

#Remove the HAVE_MINCORE because inside SGX doesn't exist mincore() function
sed -i 's/#define HAVE_MINCORE/\/\/#define HAVE_MINCORE/g' include/config.h
