#cd /media/DATA2T8GB/infracore/ ; ./pushgitchanges.sh ; cd /media/3734293d-94a8-47ec-a917-18019b14e57b/infracore/ ; git pull ; cd ~/infracore ; git pull

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

cd /media/DATA2T8GB/infracore/ ; ./pushgitchanges.sh ; cd /media/disk/infracore/ ; git pull ; cd ~/infracore ; git pull
