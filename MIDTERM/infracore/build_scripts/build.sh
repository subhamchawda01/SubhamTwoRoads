#!/bin/bash -l

set -e

export PROJECT_DIR=$(dirname $0)/..

source $PROJECT_DIR/build_scripts/common.sh

# Check if changelog impacts the build artifacts
. $PROJECT_DIR/build_scripts/check_build.sh
skip_build

cleanup_workspace
clone_and_compile_repo dvccode master release

b2 release -j$CONCURRENCY --install_loc=$DEPS_INSTALL CombinedShmWriter mktDD CombinedShmMulticaster cme_ilink_ors
b2 release -j$CONCURRENCY --install_loc=$DEPS_INSTALL --only_target=Tests
