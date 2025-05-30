#!/bin/bash
#delete older than 6 hours
if [ -d /spare/local/$USER/RS ] ; then
find /spare/local/$USER/RS/* -maxdepth 1 -type d -mmin +360 -exec rm -rf {} \;
find /spare/local/$USER/RS -type f -atime +1 -exec rm -f {} \;
fi

if [ -d /spare/local/$USER/RSM ] ; then
find /spare/local/$USER/RSM/* -type d -mtime +5 -exec rm -rf {} \;
find /spare/local/$USER/RSM -type f -atime +3 -exec rm -f {} \;
fi

# WFRM is created by wf type 6 refresh_models
if [ -d /spare/local/$USER/WFRM ] ; then
find /spare/local/$USER/WFRM/* -type d -mtime +3 -exec rm -rf {} \;
fi

# WFRM and WF are created by wf create_config_from_ifile
if [ -d /spare/local/$USER/WF ] ; then
find /spare/local/$USER/WF/* -mindepth 1 -type d -mtime +3 -exec rm -rf {} \;
fi

# Created by find_best_pnl_model
if [ -d /spare/local/$USER/PNL_MODELLING_PREPROCESSING ] ; then
find /spare/local/$USER/PNL_MODELLING_PREPROCESSING/* -mindepth 1 -type d -mtime +1 -exec rm -rf {} \;
fi
# Delete all old files directly present in /spare/local/$USER
if [ -d /spare/local/$USER ] ; then
find /spare/local/$USER/* -maxdepth 1 -type f -mtime +20 -exec rm -rf {} \;
fi

if [ -d /spare/local/$USER/ParamRolls ] ; then
find /spare/local/$USER/ParamRolls/* -type d -mtime +3 -exec rm -rf {} \;
fi

if [ -d /spare/local/$USER/FBPA ] ; then
find /spare/local/$USER/FBPA/*/*/*/*/* -type d -mtime +10 -exec rm -rf {} \;
fi

if [ -d /spare/local/$USER/PerturbedModelTests ] ; then 
find /spare/local/$USER/PerturbedModelTests/* -type d -mtime +5 -exec rm -rf {} \;
find /spare/local/$USER/PerturbedModelTests -type f -atime +4 -exec rm -f {} \;
fi

if [ -d /spare/local/$USER/PerturbAddedIndicator ] ; then 
find /spare/local/$USER/PerturbAddedIndicator/* -type d -mtime +5 -exec rm -rf {} \;
find /spare/local/$USER/PerturbAddedIndicator -type f -atime +4 -exec rm -f {} \;
fi

for i in 0 1 2 3; do
if [ -d /spare$i/local/$USER/RS ] ; then
find /spare$i/local/$USER/RS/* -maxdepth 1 -type d -mmin +360 -exec rm -rf {} \;
find /spare$i/local/$USER/RS -type f -atime +1 -exec rm -f {} \;
fi

if [ -d /spare$i/local/$USER/RSM ] ; then
find /spare$i/local/$USER/RSM/* -type d -mtime +5 -exec rm -rf {} \;
find /spare$i/local/$USER/RSM -type f -atime +2 -exec rm -f {} \;
fi

if [ -d /spare$i/local/$USER/PerturbedModelTests ] ; then 
find /spare$i/local/$USER/PerturbedModelTests/* -type d -mtime +5 -exec rm -rf {} \;
find /spare$i/local/$USER/PerturbedModelTests -type f -atime +4 -exec rm -f {} \;
fi

if [ -d /spare$i/local/$USER/PerturbAddedIndicator ] ; then 
find /spare$i/local/$USER/PerturbAddedIndicator/* -type d -mtime +5 -exec rm -rf {} \;
find /spare$i/local/$USER/PerturbAddedIndicator -type f -atime +4 -exec rm -f {} \;
fi

# Delete all old files directly present in /spare/local/$USER
if [ -d /spare$i/local/$USER ] ; then
find /spare$i/local/$USER/* -maxdepth 1 -type f -mtime +20 -exec rm -rf {} \;
fi
done

if [ -d /spare/local/$USER/lrdbdata ] ; then
find /spare/local/$USER/lrdbdata/ -mindepth 1 -type d -mtime +1 -empty -delete;
find /spare/local/$USER/lrdbdata -type f -atime +5 -exec rm -f {} \;
fi

if [ -d /spare/local/$USER/pnltemp ] ; then
find /spare/local/$USER/pnltemp/ -mindepth 1 -type d -mtime +1 -empty -delete;
find /spare/local/$USER/pnltemp -type f -atime +5 -exec rm -f {} \;
fi
