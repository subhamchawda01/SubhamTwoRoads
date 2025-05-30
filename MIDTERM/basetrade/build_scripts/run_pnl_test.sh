#!/bin/bash -l

set -e

export PROJECT_DIR=$(dirname $0)/..
export DEPS_INSTALL=$PROJECT_DIR/build
export STABLE_BUILD_DIR=$DEPS_INSTALL/stable
export VENV_DIR=$PROJECT_DIR/venv
export CC=gcc
export PYTHONPATH=$PYTHONPATH:/usr/lib/python3/dist-packages
export CONCURRENCY=16

# Check if changelog impacts the build artifacts
. $PROJECT_DIR/build_scripts/check_build.sh
skip_build

# Setup virtualenv
rm -rf $VENV_DIR
virtualenv -p python3 $VENV_DIR
source $VENV_DIR/bin/activate

# Refer from VENV_DIR because of shebang max line length
$VENV_DIR/bin/python3 $VENV_DIR/bin/pip3 install -r requirements.txt

# TODO: Enable after first run
rm -rf $STABLE_BUILD_DIR
mkdir -p $STABLE_BUILD_DIR
cd $STABLE_BUILD_DIR
    aws s3 cp $DVC_ARTIFACTORY/basetrade/latest.tgz .
    tar xf latest.tgz
cd -

#Hack: Copy binaries from dvccode, dvctrade to basetrade for running Python tests
cp $DEPS_INSTALL/dvccode/bin/* $DEPS_INSTALL/basetrade/bin
cp $DEPS_INSTALL/dvctrade/bin/* $DEPS_INSTALL/basetrade/bin
chmod +x $DEPS_INSTALL/basetrade/bin/*

parallel -j $CONCURRENCY -a $PROJECT_DIR/Tests/data/shortcodes_to_test.txt --colsep ' ' $VENV_DIR/bin/python3 $PROJECT_DIR/Tests/scripts/run_tests.py -t GLOBAL -f $STABLE_BUILD_DIR/build/basetrade/bin/sim_strategy -s {1}
