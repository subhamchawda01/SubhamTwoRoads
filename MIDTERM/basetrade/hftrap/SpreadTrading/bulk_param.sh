awk -v "ID=$1" '{system("bash $HOME/basetrade/hftrap/SpreadTrading/create_params.sh "$1" "$2" "ID" > $HOME/SpreadTraderParams/params/"$1"_"$2"_"ID)}' $2

ls ~/SpreadTraderParams/params/*_$1 | sort -u > $HOME/SpreadTraderParams/master/masterparam_$1
