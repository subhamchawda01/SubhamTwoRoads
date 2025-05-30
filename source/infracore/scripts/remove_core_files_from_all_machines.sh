
for i in `cat /spare/local/files/prod_ip_list`  ; do 
  ssh -n -f $i "/home/dvcinfra/LiveExec/scripts/remove_core_files.sh >/dev/null 2>&1 &"  ; 
done
