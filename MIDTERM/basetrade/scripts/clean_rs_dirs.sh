#!/bin/bash
#delete older than 6 hours
if [ -d /spare/local/$USER/RS ] ; then
find /spare/local/$USER/RS/ -mindepth 1 -type d -mmin +360 -empty -delete;
find /spare/local/$USER/RS -type f -atime +2 -exec rm -f {} \;
fi

if [ -d /spare/local/$USER/RCI ] ; then
find /spare/local/$USER/RCI/ -mindepth 1 -type d -mtime +2 -empty -delete;
find /spare/local/$USER/RCI -type f -atime +2 -exec rm -f {} \;
fi

if [ -d /spare/local/$USER/AMF ] ; then
find /spare/local/$USER/AMF/ -mindepth 1 -type d -mtime +2 -empty -delete;
find /spare/local/$USER/AMF -type f -atime +2 -exec rm -f {} \;
fi

if [ -d /spare/local/$USER/RSM ] ; then
find /spare/local/$USER/RSM/ -mindepth 1 -type d -mtime +2 -empty -delete;
find /spare/local/$USER/RSM -type f -atime +2 -exec rm -f {} \;
fi

if [ -d /spare/local/$USER/FBP ] ; then
find /spare/local/$USER/FBP/ -mindepth 1 -type d -mtime +2 -empty -delete;
find /spare/local/$USER/FBP -type f -atime +2 -exec rm -f {} \;
fi

if [ -d /spare/local/$USER/FBPA ] ; then 
find /spare/local/$USER/FBPA/ -mindepth 1 -type d -mtime +2 -empty -delete;
find /spare/local/$USER/FBPA -type f -atime +2 -exec rm -f {} \;
fi

if [ -d /spare/local/$USER/PerturbedModelTests ] ; then 
find /spare/local/$USER/PerturbedModelTests/ -mindepth 1 -type d -mtime +10 -empty -delete;
find /spare/local/$USER/PerturbedModelTests -type f -atime +10 -exec rm -f {} \;
fi

if [ -d /spare/local/$USER/ParamRolls ] ; then 
find /spare/local/$USER/ParamRolls/ -mindepth 1 -type d -mtime +1 -empty -delete;
find /spare/local/$USER/ParamRolls -type f -atime +5 -exec rm -f {} \;
fi

if [ -d /spare/local/$USER/study_of_learning_methods ] ; then
find /spare/local/$USER/study_of_learning_methods/ -mindepth 1 -type d -mtime +10 -empty -delete;
find /spare/local/$USER/study_of_learning_methods -type f -atime +10 -exec rm -f {} \;
fi

if [ -d /spare/local/$USER/retlrdbdata ] ; then
find /spare/local/$USER/retlrdbdata/ -mindepth 1 -type d -mtime +1 -empty -delete;
find /spare/local/$USER/retlrdbdata -type f -atime +5 -exec rm -f {} \;
fi

if [ -d /spare/local/$USER/lrdbdata ] ; then
find /spare/local/$USER/lrdbdata/ -mindepth 1 -type d -mtime +1 -empty -delete;
find /spare/local/$USER/lrdbdata -type f -atime +5 -exec rm -f {} \;
fi

if [ -d $HOME/locks ] ; then
find $HOME/locks/*_virtuallock_dont_delete_this -type f -mtime +1 -exec rm -f {} \;
fi

