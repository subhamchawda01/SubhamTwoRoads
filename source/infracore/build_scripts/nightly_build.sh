#!/bin/bash -l

set -e

export PROJECT_DIR=$(dirname $0)/..

# Check if changelog impacts the build artifacts
. $PROJECT_DIR/build_scripts/check_build.sh
skip_build

source $PROJECT_DIR/build_scripts/common.sh

# Build once for debug target
cleanup_workspace
clone_and_compile_repo dvccode master debug
b2 debug -j$CONCURRENCY --install_loc=$DEPS_INSTALL

# Rebuild for release target
cleanup_workspace # Run build again for release target to check for consistency
clone_and_compile_repo dvccode master release
b2 release -j$CONCURRENCY --install_loc=$DEPS_INSTALL