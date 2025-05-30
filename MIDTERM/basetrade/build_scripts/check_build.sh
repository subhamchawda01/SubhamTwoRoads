#!/bin/bash

set -e

export PROJECT_DIR=$(dirname $0)/..

# Exit build based on changelog
function skip_build()
{
    REF='origin/devmodel'
    TOTAL_FILES_CHANGED=`git diff --name-only $REF | wc -l`
    VALID_FILES_CHANGED=`git diff --name-only $REF | grep -v -f $PROJECT_DIR/build_scripts/excluded_folders.txt | wc -l`
    if [ $TOTAL_FILES_CHANGED != "0" ] && [ $VALID_FILES_CHANGED == "0" ]
    then
        echo "Skipping build because no change impacting build artifacts were made"
        exit 0
    fi
}
