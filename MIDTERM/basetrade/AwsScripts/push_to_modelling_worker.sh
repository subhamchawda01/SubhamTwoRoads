#!/bin/bash

cd ~/modelling/ ; 
git add . ; 
git commit -am "auto commit from push_to_modelling_worker.sh" ; 
git pull --no-edit ; 
if [ $? -ne 0 ] ; then git pull; fi;  #no-edit not supported on ny11 git
git push origin master;
