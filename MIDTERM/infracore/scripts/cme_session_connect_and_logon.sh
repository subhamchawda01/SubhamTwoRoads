#!/bin/bash

/home/dvcinfra/LiveExec/OrderRoutingServer/ors_control.pl CME $1 START 
sleep 15 ;
/home/dvcinfra/LiveExec/OrderRoutingServer/ors_control.pl CME $1 LOGIN
