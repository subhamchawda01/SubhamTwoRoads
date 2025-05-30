grep SUM $HOME/ProgressReport/infrapr_*.txt | awk -F' ' '{print $1,$5}' | sed 's#'$HOME'/ProgressReport/infrapr_##' | sed 's#.txt:SUM:##' > $HOME/_temp_inf.txt
$HOME/infracore/scripts/plotgraphdays.pl $HOME/_temp_inf.txt 1 2
tail -1 $HOME/_temp_inf.txt
