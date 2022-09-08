#!/bin/sh
find . -type d -name third_party -prune -o -name '*.cpp' -o -name '*.h' | xargs clang-format -i -style=file
