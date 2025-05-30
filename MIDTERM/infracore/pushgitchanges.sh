#!/bin/bash

USAGE="$0 BRANCH";
if [ $# -ne 1 ] ; 
then 
    echo $USAGE;
    exit;
fi

BRANCH=$1; shift;

git push origin $BRANCH
