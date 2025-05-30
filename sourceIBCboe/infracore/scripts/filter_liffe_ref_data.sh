>/home/dvcinfra/liffe-ref.txt


export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

cd /home/dvcinfra/NYSE_LIFFE_REF/

gunzip -df *.gz

YYYYMMDD=`date +"%d%m%y"`

for i in `ls /home/dvcinfra/NYSE_LIFFE_REF/ | grep "$YYYYMMDD"`; do echo $i; done

for i in `ls /home/dvcinfra/NYSE_LIFFE_REF/ | grep "$YYYYMMDD"`; do /home/dvcinfra/infracore_install/bindebug/generate_liffe_refdata /home/dvcinfra/NYSE_LIFFE_REF/$i >> /home/dvcinfra/liffe-ref.txt ; done

cat /home/dvcinfra/liffe-ref.txt | awk -F"~" '{print $5 "~" $6}' > /spare/local/files/LIFFE/liffe-ref.txt
