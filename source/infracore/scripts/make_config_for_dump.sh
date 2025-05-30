if [ $# -lt 2 ]
then
	echo "Usage: $0 Exchange_name [types_of_raw_data_to_be_dumped]"
	exit 2
fi

get_incremental_a=false
get_incremental_b=false
get_reference=false

exchange_name=$1
shift

case $exchange_name in
	"NTP") make_config_command="cat /spare/local/files/BMF/ntp-mcast.txt | grep -v \"#\" | sed '/^[[:space:]]*$/d' |awk '{print \$2,\$3,\$4,\$5}' | grep -v Snapshot | sed 's/Increment/incremental/g' | sed 's/Reference/reference/g' | sed 's/A/side_a/g' | awk '{print \$1,\$2,\$3,\$4,NR}' | sed '1iNTP'";;
	"PUMA") make_config_command="cat /spare/local/files/BMF/puma-mcast.txt | grep -v \"#\" | sed '/^[[:space:]]*$/d' |awk '{print \$2,\$3,\$4,\$5}' | grep -v Snapshot | sed 's/Increment/incremental/g' | sed 's/Reference/reference/g' | sed 's/A/side_a/g' | awk '{print \$1,\$2,\$3,\$4,NR}' | sed '1iPUMA'";;
	"CMEMDP") make_config_command="cat /spare/local/files/CMEMDP/cme-mcast.txt | grep -v \"#\" | grep -v S | sed 's/I/incremental/g' | sed 's/N/reference/g' | sed 's/A/side_a/g'| sed 's/B/side_b/g' |awk '{print \$2, \$3, \$4, \$5, NR}' | sed '1iCMEMDP'";;
	"HKOMD") make_config_command="cat /spare/local/files/HKEX/hkex_orion_mcast.txt | grep -v \"#\" | awk '{if(\$1 == \"R\") print \$1, \"A\", \$6, \$7, \"newlineR\", \$9, \$10; else print \$1, \"A\" ,\$6, \$7, \"newlineM\", \$9, \$10;}' | sed 's/newlineR/\nR B/g'  | sed 's/newlineM/\nM B/g' | sed 's/R/reference/g' | sed 's/M/incremental/g' | sed 's/A/side_a/g' | sed 's/B/side_b/g' | awk '{print \$1, \$2, \$3, \$4, NR}' | sed '1iHKOMD'";;
	"RTS") make_config_command=" cat /spare/local/files/RTS/rts-mcast.txt | grep -v \"#\" | sed 's/NS/reference/g' | grep -v S | grep -v N | sed  's/I/incremental/g' | sed 's/A/side_a/g' | sed 's/B/side_b/g' |  awk '{print \$2, \$3, \$5, \$6, NR}' | sed '1iRTS'";;
	"LIFFE") make_config_command="cat /spare/local/files/LIFFE/liffe-mcast.txt | grep -v \"#\" | head -n -3 | awk '{print \$2,\$3,\$4,\$5,NR}' | sed 's/I/incremental/g'| sed 's/N/reference/g' | sed 's/A/side_a/g' | sed 's/B/side_b/g' | sed '1iLIFFE'";;
	"MICEX_CR") make_config_command="cat /spare/local/files/MICEX/micex-mcast.txt | grep -v \"#\" | grep -v S | grep CR | awk '{ print \$2,\$3,\$5,\$6}' | sed 's/I/incremental/g' | sed 's/N/reference/g' | sed 's/A/side_a/g' | sed 's/B/side_b/g' | sed '/^\s*$/d' |  awk '{ print \$1,\$2,\$3,\$4, NR}' | sed '1iMICEX_CR'";;
	"MICEX_EQ") make_config_command="cat /spare/local/files/MICEX/micex-mcast.txt | grep -v \"#\" | grep -v S | grep EQ | awk '{ print \$2,\$3,\$5,\$6}' | sed 's/I/incremental/g' | sed 's/N/reference/g' | sed 's/A/side_a/g' | sed 's/B/side_b/g' | sed '/^\s*$/d' |  awk '{ print \$1,\$2,\$3,\$4, NR}' | sed '1iMICEX_EQ'";;
	"EOBI") make_config_command="cat /spare/local/files/EUREX/eobi-mcast.txt | grep -v \"#\"  | awk '{print \$1, \$2, \$3, \$4, \$5}' | sed 's/ 0 / incremental /' | sed 's/ 1 / reference /' | sed 's/A/side_a/g' | sed 's/B/side_b/g'| awk '{print \$2, \$3, \$4, \$5, NR}' | sed '1iEOBI'";;
	"TMX") make_config_command="cat /spare/local/files/TMX/tmx-prod-mcast-addr.txt | grep -v \"#\" | sed 's/L2/incremental/g' | sed 's/A/side_a/g' | sed 's/B/side_b/g' | awk '{print \$1,\$2,\$3,\$4,NR}'| sed '1iTMX'";;
	"CSM") make_config_command="cat /spare/local/files/CSM/csm-mcast.txt | grep -v \"#\" | sed 's/I/incremental/g' | sed 's/N/reference/g' | sed 's/A/side_a/g'| sed 's/B/side_b/g' | awk '{print \$2, \$3, \$4, \$5, NR}' | sed '1iCSM'";;
	"ICE_PL") make_config_command= "cat /spare/local/files/ICE/ice-mcast-mktdd.txt | grep -v \"#\" | grep -v S | sed 's/I/incremental/g' | grep P | awk '{print \$2, \"side_a\", \$4, \$5, NR}'| sed '1iICE_PL'";;
	"ICE_FOD") make_config_command="cat /spare/local/files/ICE/ice-mcast-mktdd.txt | grep -v \"#\" | grep -v S | sed 's/I/incremental/g' | grep F | awk '{print \$2, \"side_a\", \$4, \$5, NR}'| sed '1iICE_FOD'";;
	"ASX") make_config_command="cat /spare/local/files/ASX/asx-mcast.txt | grep -v \"#\" | sed '/^[[:space:]]*$/d'  | awk '{print \$1, \$2, \$3, \$4, NR}' | sed '1iASX'";;
	
	*)	echo "unexpected exchange name" 
	    exit 2
esac

if [ "$1" != "side_a" ]
then
	make_config_command="$make_config_command | grep -v side_a"
fi
shift

if [ "$1" != "side_b" ]
then
	make_config_command="$make_config_command | grep -v side_b"
fi
shift

if [ $1 != "reference" ]
then
	make_config_command="$make_config_command | grep -v reference"
fi
shift

make_config_command="$make_config_command > /home/dvcinfra/${exchange_name}_raw_data_config"

eval $make_config_command
