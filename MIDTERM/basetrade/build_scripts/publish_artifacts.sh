#!/bin/bash

set -e

export PROJECT_DIR=$(dirname $0)/..
export DEPS_INSTALL=$PROJECT_DIR/build

# Check if changelog impacts the build artifacts
. $PROJECT_DIR/build_scripts/check_build.sh
skip_build

ARTIFACTS_DIR=$PROJECT_DIR/artifacts

if [ -z "${1}" ]
then
  ARTIFACT_NAME=basetrade-`date +%Y-%m-%d-%H-%M-%S`.tgz
else
  ARTIFACT_NAME=$1
fi

echo "${ARTIFACT_NAME}"

rm -rf *.tgz

#Hack: Copy binaries from dvccode, dvctrade to basetrade for running sim_strategy
cp $DEPS_INSTALL/dvccode/bin/* $DEPS_INSTALL/basetrade/bin
cp $DEPS_INSTALL/dvctrade/bin/* $DEPS_INSTALL/basetrade/bin
chmod +x $DEPS_INSTALL/basetrade/bin/*

git log -n 1 > $DEPS_INSTALL/basetrade/GIT_LOG

tar cvzf $ARTIFACT_NAME $DEPS_INSTALL/dvccode \
            $DEPS_INSTALL/dvctrade \
            $DEPS_INSTALL/baseinfra \
            $DEPS_INSTALL/basetrade

aws s3 cp $ARTIFACT_NAME $DVC_ARTIFACTORY/basetrade/
aws s3 cp $ARTIFACT_NAME $DVC_ARTIFACTORY/basetrade/latest.tgz
