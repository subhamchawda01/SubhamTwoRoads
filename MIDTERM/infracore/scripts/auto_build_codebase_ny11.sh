#!/bin/bash
source $HOME/.bashrc;

#clear everything
rm -rf /home/pengine/codebase/cvquant_install/*
rm -rf /home/pengine/codebase/fpga_setup/cvquant_install/*

#get all headers
cd ~/codebase/dvccode
git reset --hard ; git stash
git checkout master ; git pull ; b2 release headers ;

cd ~/codebase/baseinfra
git reset --hard ; git stash
git checkout master ; git pull ; b2 release headers ;

cd ~/codebase/dvctrade
git reset --hard ; git stash
git checkout master ; git pull ; b2 release headers ;

cd ~/codebase/infracore
git reset --hard ; git stash
git checkout master ; git pull; b2 release headers ;

cd ~/codebase/basetrade
git reset --hard ; git stash
git checkout devmodel ; git pull; b2 release headers ;

git reset --hard ; git stash
git checkout devtrade ; git pull; b2 release headers ;

#build targets
cd ~/codebase/dvccode
bjam release -j24 ;

cd ~/codebase/baseinfra
bjam release -j24 ;

cd ~/codebase/dvctrade
bjam release -j24 ;

cd ~/codebase/infracore
bjam release -j24 ;

cd ~/codebase/basetrade
git reset --hard ; git stash
git checkout devmodel ;
bjam release -j24 ;

git reset --hard ; git stash
git checkout devtrade ;
bjam release -j24 ;


#built everything except fpga_tradeinit

cd /home/pengine/codebase/fpga_setup/dvccode/
git reset --hard ; git stash
git checkout master ; git pull ; b2 release headers ;

cd /home/pengine/codebase/fpga_setup/baseinfra/
git reset --hard ; git stash
git checkout baseinfra_fpga ; 
git pull origin baseinfra_fpga;
git pull origin master ;  git push origin baseinfra_fpga;
b2 release headers ;

cd /home/pengine/codebase/fpga_setup/dvctrade
git reset --hard ; git stash
git checkout dvctrade_fpga;
git pull origin dvctrade_fpga; git pull origin master; git push origin dvctrade_fpga;
b2 release headers ;

cd /home/pengine/codebase/fpga_setup/fpga_devtrade
git reset --hard ; git stash
git checkout cme-fpga-devtrade; 
git pull origin cme-fpga-devtrade; git pull origin devtrade; git push origin cme-fpga-devtrade
b2 release headers ;

#build targets
cd /home/pengine/codebase/fpga_setup/dvccode/
bjam release -j24 ;

cd /home/pengine/codebase/fpga_setup/baseinfra/
bjam release -j24 ;

cd /home/pengine/codebase/fpga_setup/dvctrade
bjam release -j24 ;

cd /home/pengine/codebase/fpga_setup/fpga_devtrade
bjam release -j16 tradeinit_fpga
