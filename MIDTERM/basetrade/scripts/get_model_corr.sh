#!/bin/bash

if [ $# -lt 2 ];
then
    echo "$0 model_file_name/path date"
    exit;
fi
model_fl=$1
if [ "$model_fl" == "-st" ]
then
    shift;
    stfl=$1;
    if [ ! -e $stfl ]; then stfl=`find ~/modelling -name $stfl`; fi
    if [ ! -e $stfl ]; then echo "Cannot find $stfl in ~/modelling/!! Exiting"; exit; fi
    model_fl=`cut -d' ' -f4 $stfl`
fi


if [ ! -e "$model_fl" ]; then model_fl=`basename $model_fl`; model_fl=$(find ~/modelling -name $model_fl); fi
if [ ! -e "$model_fl" ]; then echo "Model File not Found: $model_fl"; exit; fi
shift; 
echo $model_fl
dts=$*
model_prefix=`basename $model_fl | cut -d'_' -f1-4`;
reg_params=`basename $model_fl | sed 's/.*_.*_\([0-9]*_[na][ac]_[te][0-9]\)_[0-9]\{8\}_[0-9]\{8\}_\([A-Z]\{3\}_[0-9]*_[A-Z]\{3\}_[0-9]*_[0-9]*_[0-9]*_[0-9]*\)_\(f.*\)_[A-Z]*_.*_.*$/\1/g'`
datagen_params=`basename $model_fl | sed 's/.*_.*_\([0-9]*_[na][ac]_[te][0-9]\)_[0-9]\{8\}_[0-9]\{8\}_\([A-Z]\{3\}_[0-9]*_[A-Z]\{3\}_[0-9]*_[0-9]*_[0-9]*_[0-9]*\)_.*$/\2/g'`
#filter=`basename $model_fl | sed 's/w_model_ilist_.*_\([0-9]*_[na][ac]_[te][0-9]\)_[0-9]*_[0-9]*_\([A-Z]\{3\}_[0-9]*_[A-Z]\{3\}_[0-9]*_[0-9]*_[0-9]*_[0-9]*\)_.*$/\3/g'`

if [ -z "$reg_params" ]
then
    reg_param_offset=8000
    reg_param_algo=na_t3
else
    reg_param_offset=${reg_params%%_*}; if [ "${reg_param_offset%%_*}" == "na" ];then let reg_param_offset="$reg_param_offset * 1000"; fi
    reg_param_algo=${reg_params#*_}
fi

if [ -z "$datagen_params" ]
then
    datagen_params_st_end_time="CET_800 CET_1400"
    datagen_params_tt="1000 0 0"
else
#    echo $datagen_params
    datagen_params_st_end_time=$(echo $datagen_params | sed 's/\([A-Z]*_[0-9]*\)_\([A-Z]*_[0-9]*\)_\(.*\)/\1 \2/g')
    datagen_params_tt=$(echo $datagen_params | sed 's/\([A-Z]*_[0-9]*\)_\([A-Z]*_[0-9]*\)_\(.*\)/\3/g' | sed 's/_/ /g')
fi

final_reg_fl=/spare/local/$USER/reg_out_${datagen_params// /_}_${reg_params}
for dt in ${dts[*]}; 
do

    data_out=/spare/local/$USER/data_out_${model_prefix}_${datagen_params// /_}_${dt}
    reg_out=/spare/local/$USER/reg_out_${model_prefix}_${datagen_params// /_}_${reg_params}_${dt}
    
    echo "~/basetrade_install/bin/datagen $model_fl $dt $datagen_params_st_end_time 25452 $data_out $datagen_params_tt ADD_DBG_CODE -1"
    if [ ! -e $data_out ]; then ~/basetrade_install/bin/datagen $model_fl $dt $datagen_params_st_end_time 25452 $data_out $datagen_params_tt ADD_DBG_CODE -1;fi
    echo "~/basetrade_install/bin/timed_data_to_reg_data $model_fl $data_out $reg_param_offset $reg_param_algo $reg_out"
    if [ ! -e $reg_out ]; then ~/basetrade_install/bin/timed_data_to_reg_data $model_fl $data_out $reg_param_offset $reg_param_algo $reg_out; fi
    
    coeffs=($(sed -n '/INDICATOR /{s/INDICATOR \([0-9\.\-]*\) .*/\1/g;p}' $model_fl))
    awk -v A="${coeffs[*]}" -v n=${#coeffs[*]} '{
{split(A,a,/ /)}
str=$1;
x=0; 
for (i=1; i<=n; i++) {
 t=$(1+i) * a[i]; 
 str=str" "t; 
 x += t;
}
str=str" "x
print $0,str;
}' $reg_out >> ${final_reg_fl}
rm -f ${data_out};
rm -f ${reg_out};
done

printf "\n===========================\nCoefficients and Correlations: \n"
echo ${coeffs[*]}
~/basetrade_install/bin/get_dep_corr ${final_reg_fl} | sed 's/ 1 /\n/g'

rm ${final_reg_fl}
