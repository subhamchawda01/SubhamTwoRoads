#!/bin/bash
#usage <script> <pairs_file> <date> <margin>
cat $1| tr '[&]' '~' |awk '{system("bash $HOME/basetrade/hftrap/SpreadTrading/one_pair.sh "$1" "$2" '$2'")}' > tmp

python3 $HOME/basetrade/hftrap/SpreadTrading/hedge_imperfection.py --file tmp --margin $3

rm tmp
