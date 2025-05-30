#!/bin/bash -l

set -e

# Now we need dvccode and dvctrade to be present.
# in future they should probably be independent, but this allows us more flexibility for now.
export PROJECT_DIR=$(dirname $0)/..
export DEPS_INSTALL=$(realpath $PROJECT_DIR)/build
export DEPS_SOURCE=$(realpath $PROJECT_DIR)/build/src
export CONCURRENCY=16

rm -rf $DEPS_INSTALL
mkdir -p $DEPS_INSTALL $DEPS_SOURCE

# Check if changelog impacts the build artifacts
. $PROJECT_DIR/build_scripts/check_build.sh
skip_build

function clone_repo()
{
    if [ $# -lt 1 ]; then
	echo "Usage: clone_repo repo"
	exit 1
    fi

    if [ ! -d $DEPS_SOURCE/$1 ] ; then
        cd $DEPS_SOURCE
            git clone git@github.com:cvquant/$1.git
        cd -
    fi
}

EXECS_IN_DEPENDENT_REPOS='all'

function clone_and_compile_repo()
{
    if [ $# -lt 2 ]; then
    	echo "Usage: clone_and_compile_repo repo master-branch"
    	exit 1
    fi

    repo_name=$1
    branch_name=$2
    clone_repo $repo_name

    cd $DEPS_SOURCE/$repo_name
        git checkout $branch_name
        git pull origin $branch_name
        b2 release -j$CONCURRENCY --install_loc=$DEPS_INSTALL --execs=$EXECS_IN_DEPENDENT_REPOS
    cd -
}

function setup_dependency()
{
    dependency=$1
    branch=$2
    file_name="$DVC_ARTIFACTORY/$dependency/latest.tgz"

    if [[ `aws s3 ls $file_name | wc -l` -gt 0 ]]
    then
        aws s3 cp $file_name $DEPS_INSTALL/
        tar xf $DEPS_INSTALL/latest.tgz
    else
        clone_and_compile_repo $dependency $branch
    fi
}

function build_target()
{
    b2 release -j$CONCURRENCY --install_loc=$DEPS_INSTALL --only_target=$1
}

function build()
{
    b2 release -j$CONCURRENCY --install_loc=$DEPS_INSTALL $1
}

setup_dependency dvccode master
setup_dependency baseinfra master
setup_dependency dvctrade master

build_target Tests
build_target InitLogic

build get_periodic_volume_on_day