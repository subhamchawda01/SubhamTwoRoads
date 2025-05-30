#!/bin/bash
#delete old files/directories on aws
if [ -d /spare/local/logs/tradelogs ] ; then
find /spare/local/logs/tradelogs/ -type d -mtime +2 -exec rm -rf {} \;
find /spare/local/logs/tradelogs -type f -atime +2 -exec rm -f {} \;
fi

if [ -d /spare/local/logs/datalogs ] ; then
find /spare/local/logs/datalogs/ -type d -mtime +2 -exec rm -rf {} \;
find /spare/local/logs/datalogs -type f -atime +2 -exec rm -f {} \;
fi

if [ -d /spare/local/$USER/RS ] ; then
find /spare/local/$USER/RS/ -type d -mmin +360 -exec rm -rf {} \;
find /spare/local/$USER/RS -type f -atime +2 -exec rm -f {} \;
fi

if [ -d /spare/local/$USER/RCI ] ; then
find /spare/local/$USER/RCI/ -type d -mtime +2 -exec rm -rf {} \;
find /spare/local/$USER/RCI -type f -atime +2 -exec rm -f {} \;
fi

if [ -d /spare/local/$USER/RSM ] ; then
find /spare/local/$USER/RSM/ -type d -mtime +2 -exec rm -rf {} \;
find /spare/local/$USER/RSM -type f -atime +2 -exec rm -f {} \;
fi

if [ -d /spare/local/$USER/GSW ] ; then
find /spare/local/$USER/GSW/ -type d -mtime +5 -exec rm -rf {} \;
find /spare/local/$USER/GSW -type f -atime +5 -exec rm -f {} \;
fi

if [ -d /spare/local/$USER/PerturbAddedIndicator ] ; then
find /spare/local/$USER/PerturbAddedIndicator/ -type d -mtime +10 -exec rm -rf {} \;
find /spare/local/$USER/PerturbAddedIndicator -type f -atime +10 -exec rm -f {} \;
fi

if [ -d /spare/local/$USER/PerturbReplacedIndicator ] ; then
find /spare/local/$USER/PerturbReplacedIndicator/ -type d -mtime +5 -exec rm -rf {} \;
find /spare/local/$USER/PerturbReplacedIndicator -type f -atime +5 -exec rm -f {} \;
fi

if [ -d /spare/local/$USER/FBP ] ; then
find /spare/local/$USER/FBP/ -type d -mtime +2 -exec rm -rf {} \;
find /spare/local/$USER/FBP -type f -atime +2 -exec rm -f {} \;
fi

if [ -d /spare/local/$USER/FBPA ] ; then 
find /spare/local/$USER/FBPA/ -type d -mtime +10 -exec rm -rf {} \;
find /spare/local/$USER/FBPA -type f -atime +10 -exec rm -f {} \;
fi

if [ -d /spare/local/$USER/PerturbedModelTests ] ; then 
find /spare/local/$USER/PerturbedModelTests/ -type d -mtime +10 -exec rm -rf {} \;
find /spare/local/$USER/PerturbedModelTests -type f -atime +10 -exec rm -f {} \;
fi
