#!/usr/bin/env bash
# Usage:
#  tpal.sh <path to tpal> <path to mcsl> <path to nautilus>

TPAL_PATH=$1
TPAL_RUNTIME_PATH="${TPAL_PATH}/runtime/"
TPAL_RUNTIME_HEADER_FILES="${TPAL_RUNTIME_PATH}/include/*.hpp"
TPAL_RUNTIME_BENCH_HEADER_FILES="${TPAL_RUNTIME_PATH}/bench/*.hpp"
TPAL_RUNTIME_BENCH_ASM_FILES="${TPAL_RUNTIME_PATH}/bench/*_manual.s"

MCSL_PATH=$2
MCSL_INCLUDE_PATH="$MCSL_PATH/include/"
MCSL_HEADER_FILES="$MCSL_INCLUDE_PATH/*.hpp"

NAUTILUS_PATH=$3
NAUTILUS_TPAL_INCLUDE_PATH="${NAUTILUS_PATH}/include/rt/tpal/"
NAUTILUS_TPAL_BENCH_PATH="${NAUTILUS_PATH}/src/rt/tpal/"
NAUTILUS_MCSL_INCLUDE_PATH="${NAUTILUS_PATH}/include/rt/mcsl/"

# Set symlinks for tpal header files
for f in $TPAL_RUNTIME_HEADER_FILES
do
    ln -sf $f ${NAUTILUS_TPAL_INCLUDE_PATH}/$(basename $f)
done

# Set symlinks for tpal benchmark header files
for f in $TPAL_RUNTIME_BENCH_HEADER_FILES
do
    ln -sf $f ${NAUTILUS_TPAL_BENCH_PATH}/$(basename $f)
done

# Set symlinks for tpal benchmark assembly files
for f in $TPAL_RUNTIME_BENCH_ASM_FILES
do
    ln -sf $f ${NAUTILUS_TPAL_BENCH_PATH}/$(basename $f)
done

# Set symlinks for mcsl header files
for f in $MCSL_HEADER_FILES
do
    ln -sf $f ${NAUTILUS_MCSL_INCLUDE_PATH}/$(basename $f)
done
