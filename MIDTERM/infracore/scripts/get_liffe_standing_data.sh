# Get the needed files from the NewEdge ftp.

rm -rf /home/dvcinfra/NYSE_LIFFE_REFDATA/

mkdir -p /home/dvcinfra/NYSE_LIFFE_REFDATA/
cd /home/dvcinfra/NYSE_LIFFE_REFDATA/

HOST='156.48.55.1';
USER='xdplive';
PASSWD='xdplive';

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


YYYYMMDD=`date +"%d%m%y"`

ftp -n $HOST <<SCRIPT
user $USER $PASSWD
binary

get "nyseliffe_stdata_eqt_A_"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_eqt_B_"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_eqt_C_"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_eqt_D_"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_eqt_F_"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_eqt_G_"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_eqt_H_"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_eqt_K_"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_eqt_O_"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_eqt_P_"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_eqt_Z_"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_fin_C_"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_fin_H_"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_fin_J_"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_fin_L_"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_fin_M_"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_fin_O_"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_fin_S_"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_fin_X_"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_fin_Y_"$YYYYMMDD".xml.gz"

quit
SCRIPT

scp * 10.23.52.91:/home/dvcinfra/NYSE_LIFFE_REF/
