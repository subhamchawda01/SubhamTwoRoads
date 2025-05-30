SCRIPTS_PATH=$HOME/basetrade/scripts
for file in `cat $SCRIPTS_PATH/file_list.txt`; do $SCRIPTS_PATH/nas_to_hs1_sync.sh $file; done
