USAGE="$0 Shortcode YYYYMMDD";

if [ $# -ne 2 ] ; then
  echo $USAGE
fi

/home/dvcinfra/infracore_install/bin/ors_binary_reader $1 $2 | grep "ORR: Exec" | grep -v IntExec | awk '{if( $8 == "B") {print $2, 0, $28, $4, $16} else { print $2, 1, $28, $4, $16} }' | sed 's/ /\x01/g'
