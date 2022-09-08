#!/bin/sh
set -e
if [ ! -d "$HOME/cache" ]; then
  mkdir $HOME/cache;
fi

CTEST_EXEC="$HOME/cache/bin/ctest"

# check to see if CMake is cached
if [[ ! -f "$CTEST_EXEC" || ! "$($CTEST_EXEC --version)" =~ "3.2.3" ]]; then
  wget --no-check-certificate https://cmake.org/files/v3.2/cmake-3.2.3.tar.gz -O /tmp/cmake.tar.gz;
  tar -xzvf /tmp/cmake.tar.gz -C /tmp;
  cd /tmp/cmake-3.2.3;
  ./configure --prefix=$HOME/cache;
  make -j2;
  make install;
else
  echo "Using cached bin dir: $HOME/cache/bin";
  ls -la $HOME/cache/bin;
  echo "Using cached CMake installation:";
  cmake -version;
fi
