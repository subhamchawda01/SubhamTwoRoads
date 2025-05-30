#!/bin/bash
source $HOME/.bashrc; 
source $HOME/.bash_aliases; 
cd ~/basetrade; 
git pull && git submodule foreach git pull; 
b2 release link=static -j12;
$HOME/basetrade/scripts/setup_modelling_live_execs.sh >/home/dvctrader/out.txt
