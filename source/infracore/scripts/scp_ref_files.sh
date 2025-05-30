#!/bin/bash
#arg $1 = exchange CME OR EUREX OR BMF
#arg $2 = ip of host from which it is run, so that it is not scp-ed the files to itself

#define array of hosts where we need to scp the files
host_to_scp=(
  
  10.23.199.51
  10.23.199.52
  10.23.199.53
  10.23.199.54
  10.23.199.55
  10.23.142.51
  10.23.196.51
  10.23.196.52
  10.23.196.53
  10.23.196.54
  10.23.200.51
  10.23.200.52
  10.23.200.53
  10.23.200.54
  10.23.182.51 #sdv-tor-srv11
  10.23.182.52 #sdv-tor-srv12  
  10.23.23.11 #sdv-bmf-srv11  
  10.23.23.12 #sdv-bmf-srv12 
  10.220.40.1 #sdv-bmf-srv13
)


export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


case $1 in
EUREX)
    for host in "${host_to_scp[@]}"
    do
      echo "scp /spare/local/files/EUREX/eurex-data-mcast.txt dvcinfra@$host:/spare/local/files/EUREX/eurex-data-mcast.txt" ;
      scp /spare/local/files/EUREX/eurex-data-mcast.txt dvcinfra@$host:/spare/local/files/EUREX/eurex-data-mcast.txt ;
      echo "scp /spare/local/files/EUREX/eurex-prod-codes.txt dvcinfra@$host:/spare/local/files/EUREX/eurex-prod-codes.txt" ;
      scp /spare/local/files/EUREX/eurex-prod-codes.txt dvcinfra@$host:/spare/local/files/EUREX/eurex-prod-codes.txt ;
    done
;;
CME)
 #trim the cme ref files
    today=`date '+%Y%m%d'`
  
    #read shortcodes list and get its symbol 
    for a in `cat /spare/local/files/CME/cme-shortcodes.txt `
      do $HOME/LiveExec/bin/get_exchange_symbol $a $today 2>/dev/null; echo "" 
      done | sed '/^[ \t]*$/d' > /tmp/regex_for_cme_ref
    

    for symbol in `cat /spare/local/files/CME/cme-ref.txt  | tr '\t' ' ' | awk '{print $2}'`
    do 

       basecode=${symbol:0:2} ;

       if [ "$basecode" == "CL" ] || [ "$basecode" == "GC" ] 
       then 

          echo $symbol >> /tmp/regex_for_cme_ref 

       fi
  
   done

    #filter ref file
    grep -w -f /tmp/regex_for_cme_ref /spare/local/files/CME/cme-ref.txt > /spare/local/files/CME/cme-ref.txt_min
    
    #filter mcast file
    awk '{print "^"$3}' /spare/local/files/CME/cme-ref.txt_min | sort | uniq  > /tmp/regex_for_cme_ref
    grep -w -f /tmp/regex_for_cme_ref /spare/local/files/CME/cme-mcast.txt > /spare/local/files/CME/cme-mcast.txt_min
    rm /tmp/regex_for_cme_ref
    
    for host in "${host_to_scp[@]}"
    do
      if [ "$host" != "10.23.196.52" ] #don't copy at self (for others this is ok, but here the files are different. hence it is incorrect to scp to self)
      then
      echo "scp /spare/local/files/CME/cme-ref.txt_min dvcinfra@$host:/spare/local/files/CME/cme-ref.txt" ;
      scp /spare/local/files/CME/cme-ref.txt_min dvcinfra@$host:/spare/local/files/CME/cme-ref.txt ;
      echo "scp /spare/local/files/CME/cme-mcast.txt_min dvcinfra@$host:/spare/local/files/CME/cme-mcast.txt"
      scp /spare/local/files/CME/cme-mcast.txt_min dvcinfra@$host:/spare/local/files/CME/cme-mcast.txt ;
      fi
    done 
;;
NTP )
    for host in "${host_to_scp[@]}"
    do
      echo "scp /spare/local/files/BMF/ntp-ref.txt_min dvcinfra@$host:/spare/local/files/BMF/ntp-ref.txt" ;
      scp /spare/local/files/BMF/ntp-ref.txt dvcinfra@$host:/spare/local/files/BMF/ntp-ref.txt ;
    done
;;
BMF )
    for host in "${host_to_scp[@]}"
    do
      echo "scp /spare/local/files/BMF/bmf-ref.txt dvcinfra@$host:/spare/local/files/BMF/bmf-ref.txt" ;
      scp /spare/local/files/BMF/bmf-ref.txt dvcinfra@$host:/spare/local/files/BMF/bmf-ref.txt ;
      echo "scp /spare/local/files/BMF/bmf-mcast.txt dvcinfra@$host:/spare/local/files/BMF/bmf-mcast.txt" ;
      scp /spare/local/files/BMF/bmf-mcast.txt dvcinfra@$host:/spare/local/files/BMF/bmf-mcast.txt ;
    done
;;
*)
    echo "No arguments provided: Usage ./script <EXCHANGE>"
    exit 0
;;
esac
