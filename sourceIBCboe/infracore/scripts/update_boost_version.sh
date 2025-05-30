#!/bin/bash

USAGE="$0 TRD_MAC ";
if [ $# -ne 1 ] ; 
then 
    echo $USAGE;
    exit;
fi

TRD_MAC=$1; shift;
TRD_USER=$USER

BOOST_BASE_DIR=/apps/boost
NEW_VER=boost_1_47_0
OLD_VER=boost_1_45_0

rsync -avz $BOOST_BASE_DIR/$NEW_VER $TRD_USER@$TRD_MAC:$BOOST_BASE_DIR/
ssh $TRD_USER@$TRD_MAC rm $BOOST_BASE_DIR/root
ssh $TRD_USER@$TRD_MAC ln -s $BOOST_BASE_DIR/$NEW_VER $BOOST_BASE_DIR/root
ssh $TRD_USER@$TRD_MAC rm -rf $BOOST_BASE_DIR/$OLD_VER
