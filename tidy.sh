#!/bin/sh

if [ -f Makefile ]; then
    make clean
fi

rm -rf \
   build/ \
   _deps/ \
   CMakeFiles/ \
   CMakeCache.txt \
   Makefile \
   *.egg-info \
   *.cmake \
   test/CMakeFiles/ \
   test/*.cmake \
   test/Makefile

