
EQ_FILE=/spare/local/files/MICEX/micex-eq-ref.txt
CR_FILE=/spare/local/files/MICEX/micex-cr-ref.txt
MICEX_FILE=/spare/local/files/MICEX/micex-ref.txt
cat /dev/null >  $MICEX_FILE 
cat $EQ_FILE > $MICEX_FILE
cat $CR_FILE >> $MICEX_FILE
