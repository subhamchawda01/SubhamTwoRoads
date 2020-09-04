# !/bin/bash


generate_shortcode_position_file_OPT() {

	fo_positions_OPT_Inverted=$2;
	date=$1
	datasource_exchsymbol_file="/spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt"

	awk 'NR==FNR { fo_pos[$NF]=$(NF-1); next } { if (fo_pos[$(NF-1)]) {print fo_pos[$(NF-1)],$(NF-1),$NF} else {print "INVALID",$(NF-1),$NF}}' $datasource_exchsymbol_file $fo_positions_OPT_Inverted > OPT_exchsymbol_position

	grep -v "INVALID" OPT_exchsymbol_position | awk '{print $1}' > OPT_exchsymbol

	true > OPT_exchsymbol_shortcode

	/home/pengine/prod/live_execs/get_shortcode_from_ds $date OPT_exchsymbol OPT_exchsymbol_shortcode

	awk 'NR==FNR { fo_pos[$(NF-1)]=$NF; next } { if (fo_pos[$(NF-2)]) print fo_pos[$(NF-2)],$NF}' OPT_exchsymbol_shortcode OPT_exchsymbol_position > position_to_close_OPT

}

generate_shortcode_position_file_FUT() {

	fo_positions_FUT_Inverted=$1;
	awk 'NR>1 {print $(NF-1),$NF}' $fo_positions_FUT_Inverted > position_to_close_FUT

}

if [ $# == 3 ]; then
	case "$2" in
		"FUT") 	generate_shortcode_position_file_FUT $3
		;;
		"OPT")	generate_shortcode_position_file_OPT $1 $3
		;;
	esac
else
	echo "USAGE: <script> <yyyymmdd> FUT/OPT <fo_position_FUT/OPT_Inverted>"
fi

