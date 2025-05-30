#!/bin/bash

set -e

export PROJECT_DIR=$(dirname $0)/..
export DEPS_INSTALL=$PROJECT_DIR/build
export CONCURRENCY=16

# Clone dvctrade repo
rm -rf dvctrade
git clone git@github.com:cvquant/dvctrade.git

# Copy build folder from baseinfra build_dir to dvctrade build_dir
cp -r $DEPS_INSTALL dvctrade

cd dvctrade
    # Build dvctrade
    b2 release -j$CONCURRENCY --install_loc=build
cd -

# Clone basetrade repo
rm -rf basetrade
git clone git@github.com:cvquant/basetrade.git

# Copy build folder from dvctrade build_dir to basetrade build_dir
cp -r dvctrade/build basetrade

cd basetrade
    # Build sim_strategy
    b2 release -j$CONCURRENCY --install_loc=build sim_strategy

    # Trigger run_pnl_tests.sh script in basetrade folder
    bash -l build_scripts/run_pnl_test.sh
cd -
