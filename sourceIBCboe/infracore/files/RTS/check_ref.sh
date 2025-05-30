#!/bin/bash

if [ "`wc -l /spare/local/files/RTS/rts-ref.txt | awk '{print $1}'`" -lt 20 ]; 
then 
    cp /spare/local/files/RTS/rts-ref.txt_bak /spare/local/files/RTS/rts-ref.txt; 
    
fi