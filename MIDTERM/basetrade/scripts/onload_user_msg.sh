#!/bin/bash

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

export EF_LOG_VIA_IOCTL=1 ;

#export EF_MAX_PACKETS=50000 ; 
#export EF_MAX_RX_PACKETS=40000 ; 
#export EF_MAX_TX_PACKETS=10000 ;

#export EF_UDP_RCVBUF=80000 ;
export EF_NO_FAIL=0 ;
export EF_UDP_RECV_MCAST_UL_ONLY=1 ;

export EF_SPIN_USEC=1000000 ; 
export EF_POLL_USEC=1000000 ; 
export EF_SELECT_SPIN=1 ; 
export EF_MULTICAST_LOOP_OFF=0 ;
export EF_MAX_ENDPOINTS=1024 ;
export EF_SHARE_WITH=-1;
export EF_NAME=ORS_STACK ;

onload ~/LiveExec/bin/user_msg $@
