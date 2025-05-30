

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


if [ "`wc -l /spare/local/files/RTS/rts-ref.txt | awk '{print $1}'`" -lt 20 ]; then cp /spare/local/files/RTS/rts-ref.txt_bak /spare/local/files/RTS/rts-ref.txt; /home/circulumvite/LiveExec/scripts/sendAlert.sh "Reference file not 
generated"; fi

