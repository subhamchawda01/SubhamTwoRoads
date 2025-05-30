#!/bin/bash -l

set -e

export PROJECT_DIR=$(dirname $0)/..
export DEPS_INSTALL=$(realpath $PROJECT_DIR)/build
export DEPS_SOURCE=$(realpath $PROJECT_DIR)/build/src
export CONCURRENCY=16

rm -rf $DEPS_INSTALL
mkdir -p $DEPS_INSTALL $DEPS_SOURCE

# Check if changelog impacts the build artifacts
. $PROJECT_DIR/build_scripts/check_build.sh
skip_build

b2 release -j$CONCURRENCY --install_loc=$DEPS_INSTALL
