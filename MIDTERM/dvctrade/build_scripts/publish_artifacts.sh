#!/bin/bash

set -e

export PROJECT_DIR=$(dirname $0)/..
export DEPS_INSTALL=$PROJECT_DIR/build

# Check if changelog impacts the build artifacts
. $PROJECT_DIR/build_scripts/check_build.sh
skip_build

ARTIFACTS_DIR=$PROJECT_DIR/artifacts
ARTIFACT_NAME=dvctrade-`date +%Y-%m-%d-%H-%M-%S`.tgz

rm -rf *.tgz

git log -n 1 > $DEPS_INSTALL/dvctrade/GIT_LOG

tar cvzf $ARTIFACT_NAME $DEPS_INSTALL/dvctrade

aws s3 cp $ARTIFACT_NAME $DVC_ARTIFACTORY/dvctrade/
aws s3 cp $ARTIFACT_NAME $DVC_ARTIFACTORY/dvctrade/latest.tgz