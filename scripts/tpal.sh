#!/usr/bin/env bash

TPAL_PATH=$1
TPAL_RUNTIME_PATH="${TPAL_PATH}/runtime/"
TPAL_RUNTIME_HEADER_FILES="${TPAL_RUNTIME_PATH}/include/*.hpp"
TPAL_RUNTIME_BENCH_HEADER_FILES="${TPAL_RUNTIME_PATH}/bench/*.hpp"
TPAL_RUNTIME_BENCH_ASM_FILES="${TPAL_RUNTIME_PATH}/bench/*_manual.s"

NAUTILUS_PATH=$2
NAUTILUS_TPAL_INCLUDE_PATH="${NAUTILUS_PATH}/include/rt/tpal/"
NAUTILUS_TPAL_BENCH_PATH="${NAUTILUS_PATH}/src/rt/tpal/"

# Set symlinks for tpal header files
for f in $TPAL_RUNTIME_HEADER_FILES
do
    ln -s $f ${NAUTILUS_TPAL_INCLUDE_PATH}/$(basename $f)
done

# Set symlinks for tpal benchmark header files
for f in $TPAL_RUNTIME_BENCH_HEADER_FILES
do
    ln -s $f ${NAUTILUS_TPAL_BENCH_PATH}/$(basename $f)
done

# Set symlinks for tpal benchmark assembly files
for f in $TPAL_RUNTIME_BENCH_ASM_FILES
do
    ln -s $f ${NAUTILUS_TPAL_BENCH_PATH}/$(basename $f)
done
