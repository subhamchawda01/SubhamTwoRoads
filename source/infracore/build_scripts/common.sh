#!/bin/bash -l

set -e

export PROJECT_DIR=$(dirname $0)/..
export DEPS_INSTALL=$(realpath $PROJECT_DIR)/build
export DEPS_SOURCE=$(realpath $PROJECT_DIR)/build/src
export EXECS_IN_DEPENDENT_REPOS='all'
export CONCURRENCY=16

function cleanup_workspace()
{
    rm -rf $DEPS_INSTALL # Force rebuild
    mkdir -p $DEPS_INSTALL $DEPS_SOURCE
}

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

function clone_and_compile_repo()
{
    if [ $# -lt 3 ]; then
    echo "Usage: clone_and_compile_repo repo master-branch mode=release|debug]"
    exit 1
    fi

    repo_name=$1
    branch_name=$2
    clone_repo $repo_name

    cd $DEPS_SOURCE/$repo_name
        git checkout $branch_name
        git pull origin $branch_name
        b2 $3 -j$CONCURRENCY --install_loc=$DEPS_INSTALL --execs=$EXECS_IN_DEPENDENT_REPOS
    cd -
}