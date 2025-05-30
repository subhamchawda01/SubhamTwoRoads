#!/bin/bash


export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

find /spare/local/$USER/GSW/*/* -type d -mtime +5 -exec rm -rf {} \;

