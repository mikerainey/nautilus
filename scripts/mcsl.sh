#!/usr/bin/env bash

MCSL_PATH=$1
MCSL_INCLUDE_PATH="${MCSL_PATH}/include/"
MCSL_HEADER_FILES="${MCSL_INCLUDE_PATH}/*.hpp"

NAUTILUS_PATH=$2
NAUTILUS_MCSL_PATH="${NAUTILUS_PATH}/include/rt/mcsl/"

for f in $MCSL_HEADER_FILES
do
    ln -s $f ${NAUTILUS_MCSL_PATH}/$(basename $f)
done
