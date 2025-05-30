#!/bin/bash

for i in 1 2 3 4 5;
do 
    ssh dvctrader@10.23.74.5$i "cd basetrade; git pull; b2 release link=static -j12"
done

ssh dvctrader@sdv-crt-srv11 "cd basetrade; git pull; b2 release link=static -j12"
