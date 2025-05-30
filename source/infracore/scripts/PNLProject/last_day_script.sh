#!/bin/bash
    
yes_date=`date --date="yesterday"`
YYYYMM=`date +\%Y\%m` ;
/home/hardik/PNLProject/get_top5_gainer_loser_product.sh MONTHLY  ${YYYYMM}
