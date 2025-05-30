#!/bin/bash
#!/bin/tcsh

#day_before_expirydate_="20210224"
#echo "DATE: $day_before_expirydate_"
print_msg_and_exit() {
  echo $* ;
  exit ;
}

[ $# -eq 2 ] || print_msg_and_exit "Usage : < script > < product[SBIN] > < ratio >"

prod_=$1
ratio=$2
  if [[ "$ratio" != "" ]]; then
    echo "$prod_ - $ratio enter"
    less /spare/local/BarData/$prod_ | awk '{print $1,"\011",$2,"\011",$3,"\011",$4,"\011",$5,"\011",$6*'$ratio',$7*'$ratio',$8*'$ratio',$9*'$ratio',$10/'$ratio',$11,"\011",$12*'$ratio'}' > temp;
    mv temp "/spare/local/BarData/"$prod_;
    chown dvctrader:infra /spare/local/BarData/$prod_
  else
    echo "$prod_ - $ratio - RATIO 0"
  fi;

