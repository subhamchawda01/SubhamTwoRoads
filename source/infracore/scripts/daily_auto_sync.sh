#!/bin/bash
source $HOME/.bashrc; 
source $HOME/.bash_aliases; 
cd ~/infracore; 
git pull; 
b2 release link=static -j12;
b2 release link=static -j12;
b2 release link=static -j12;
