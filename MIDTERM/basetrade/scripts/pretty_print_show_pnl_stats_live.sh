
mkdir -p $HOME/basetrade_install/ModelScripts
cp $HOME/basetrade/ModelScripts/get_pnl_stats.pl $HOME/basetrade_install/ModelScripts/get_pnl_stats.pl

$HOME/basetrade/scripts/show_pnl_stats_day.sh | egrep "tradelogs|Abs|FinalPNL|FinalVolume:" | awk '{ if ($1=="-------") { printf "%-50s", $2; } else { if ( $1 == "Average" ) { aap=$4; } else { if ( $1 == "FinalVolume:" ) { fvol=$2; } else { pnl=$2; printf "%6d %6d %6d\n", pnl, fvol, ( pnl / aap ) ; } } } }' | sort -rg -k4
