#!/bin/bash

if [ $# -lt 1 ]; then echo "./check_trend (2/8/32)"; exit; fi
prods_listed=( 
    #CME
    "GE_0" "GE_1" "GE_2" "GE_3" "GE_4" "GE_5" "GE_6" "GE_7" "GE_8" "GE_9" "GE_10" "GE_11" "GE_12" "GE_13" "GE_14" "GE_15" "GE_16" "ZT_0"  "ZT_1"  "ZF_0"  "ZF_1"  "ZN_0"  "ZN_1"  "ZB_0"  "ZB_1"  "UB_0"  "UB_1"  "ES_0"  "ES_1"  "NQ_0"  "NQ_1"  "YM_0"  "YM_1" "6A_0"  "6A_1"  "6B_0"  "6B_1"  "6C_0"  "6C_1"  "6E_0"  "6E_1"  "6J_0"  "6J_1"  "6M_0"  "6M_1"  "6N_0"  "6N_1"  "6S_0"  "6S_1"  "CL_0"  "NG_0"  "HO_0"  "RB_0"  "GC_0"  "HG_0"  "SI_0"  "PA_0"  "PD_0" 
    "GC_0"  "GC_1"  "GC_2" "CL_0"  "CL_1"  "CL_2" 
 # EUREX 
    "FGBS_0"  "FGBS_1"  "FGBM_0"  "FGBM_1"  "FGBM_2"  "FGBL_0"  "FGBL_1"  "FGBX_0"  "FBTS_0"  "FBTP_0"  "FOAT_0"  "CONF_0"  "FESX_0"  "FDAX_0"  "FSMI_0" 
    "FXXP_0"  "FEXF_0"  "FESB_0"  "FSTB_0"  "FSTS_0"  "FSTO_0"  "FSTG_0"  "FSTI_0"  "FSTM_0"  "OKS2_0"  "FEXD_0"  "FRDX_0"  "FEU3_0" 
    
# TMX 
    "CGB_0"  "CGB_1"  "SXF_0"  "BAX_0"  "BAX_1"  "BAX_2"  "BAX_3"  "BAX_4"  "BAX_5"  "BAX_6"  "BAX_7"  "CGB_1"  "CGB_2"  "CGB_3"  "SP_CGB0_CGB1"  "SP_CGB0_CGB2"  "SP_CGB0_CGB3"  "SP_CGB1_CGB2"  "SP_CGB2_CGB3" "SP_BAX0_BAX1"  "SP_BAX0_BAX2"  "SP_BAX0_BAX3" "SP_BAX1_BAX2"  "SP_BAX2_BAX3"  "SP_BAX3_BAX4"  "SP_BAX4_BAX5"  "SP_BAX5_BAX6" "CGF_0" "CGZ_0"  
 # BMF
    "BR_IND_0"  "BR_WIN_0"  "BR_DOL_0"  "BR_DOL_1"  "BR_DOL_2"  "BR_WDO_0"  "BR_WDO_1"  "BR_WDO_2" "DI1F16"  "DI1F15"  "DI1F19"  "DI1F17"  "DI1N14"  "DI1F18"  "DI1F21"  "DI1F22"  "DI1F23" 
    "ALLL3" "RUMO3" "AMBV4"  "BBAS3"  "BBDC4"  "BISA3"  "BRAP4"  "BRFS3"  "BRKM5"  "BRML3"  "BRTO4"  "BTOW3"  "BVMF3"  "CCRO3"  "CESP6"  "CIEL3"  "CMIG4"  "CPFE3"  "CPLE6"  "CRUZ3"  "CSAN3"  "CSNA3"  "CYRE3"  "DASA3"  "DTEX3"  "ELET3"  "ELET6"  "ELPL4"  "EMBR3"  "FIBR3"  "GFSA3"  "GGBR4"  "GOAU4"  "GOLL4"  "HGTX3"  "HYPE3"  "ITSA4"  "ITUB4"  "JBSS3"  "KLBN4"  "LAME4"  "LIGT3"  "LLXL3"  "LREN3"  "MMXM3"  "MRFG3"  "MRVE3"  "NATU3"  "OGXP3"  "PCAR4"  "PDGR3"  "PETR3"  "PETR4"  "RDCD3"  "RENT3"  "RSID3"  "SANB11" "SBSP3"  "TAMM4"  "TIMP3"  "TMAR5"  "TNLP3"  "TNLP4"  "TRPL4"  "UGPA3"  "USIM3"  "USIM5"  "VAGR3"  "VALE3"  "VALE5"  "VIVT4" 
    "JFFCE_0"  "JFFCE_1" 
    "KFFTI_0"
    "LFZ_0"  "LFZ_1"
    "LFL_0"  "LFL_1"  "LFL_2"  "LFL_3"  "LFL_4"  "LFL_5"  "LFL_6"  "LFL_7"  "LFL_8"  "LFL_9"  "LFL_10" "LFL_11" "LFL_12"
    "LFR_0" 
    "LFI_0"  "LFI_1"  "LFI_2"  "LFI_3"  "LFI_4"  "LFI_5"  "LFI_6"  "LFI_7"  "LFI_8"  "LFI_9"  "LFI_10" "LFI_11" "LFI_12"
 #NASDAQ "FXE"  "IEF"  "TLT"  "USO"  "GLD"  "QQQ"  "SPY"  "IWM"  "EEM"  "EWZ"  "XLF"  "SDS"  "EWG"  "EWU"
 #RTS products "RI_0"  "Si_0" "LK_0"  "GZ_0"  "SR_0"  "GM_0"  "RN_0" 
 #MICEX products  "SBER"     "USD000000TOD"     "USD000TODTOM"     "USD000UTSTOM"     "MGNT" 
)


dir_path="/NAS1/indicatorwork/"
la=$1; shift; 
if [ $# -lt 1 ]
then
    prods=(`ls $dir_path | grep -v indicator_list | grep "_${1}_na_" | sed "s/_${1}_na_.*//g" | sort | uniq`)
    1>~/basetrade/files/IndicatorInfo/lead_lag_ind_${1}
else
    prods=($*)
fi

#prods=${prods1[*]:0:5}
last_date=`date +%Y%m%d -d "20 days ago"`
echo $last_date
lead_sec=2

mat=()
tfile=/home/dvctrader/kputta/tmpp.txt
mkdir -p `dirname $tfile`;

for p in ${prods[*]}
do
    echo $p ;
    files=`ls $dir_path/${p}_${la}_*_US_MORN_DAY_OfflineMixMMS_OfflineMixMMS/indicator_corr_record_file.txt*`
    echo $files >&2
    if [ -z "$files" ] ; then 
	echo $files "does not exist...Skiping." >&2; 
	continue; 
    fi
    
    zgrep " SimpleTrend \| SimpleTrendPort " $files > ~/tmp_trend ; # | awk '{if( $1 > '$last_date' ) print $0}' > ~/tmp_trend
    dep_p=(`awk '{print $5}' ~/tmp_trend | sort | uniq`)
    
    echo $p "==>" ${dep_p[*]} >&2
    for q in ${dep_p[*]}
    do
	t=`grep " $q " ~/tmp_trend | awk -v sum=0 -v sqsum=0 '{ sum += $3; sqsum += $3*$3 } END {if (NR>0) {mean=sum/NR; var=sqsum/NR-mean*mean;} else {mean=0; var=0;} print mean, var}'`
	echo $q $t

    done | sed 's/-//g' > $tfile
    
    m=(`awk '{sum += $2; sdsum += $3;}END{if (NR>0) {mean=sum/NR; msd=sdsum/NR; print mean, msd} else {print 99, 99}}' $tfile`)
    sd=`echo "sqrt ( ${m[1]} )" | bc -l`
    cutoff=`echo ${m[0]}" + "$sd" * "0.5 | bc -l` 
    #echo $cutoff
    echo "Cutoff for " $p $cutoff >&2
    echo $p `cat $tfile | awk '{if ( $2 > '$cutoff' ) print $1 ":" $2}'`
done > ${tfile}_res_${1}

python -c "
print '#<leading prod> <<lagging prod list>>\n#We should not use the lagging prod trend to predict the trend of leadind prod'
d={};
for l in open('"${tfile}_res_$1"').readlines():
   l = l.strip().split();
   for x in l[1:]:
       x=x.split(':')[0]
       if '_' not in x: continue
       if x in d: d[x] += [l[0]]
       else: d[x] = [l[0]]
print '\n'.join(['%s %s' % (m, ' '.join(d[m])) for m in d])
" > ~/basetrade/files/IndicatorInfo/lead_lag_ind_${1}.txt

echo "data is generated in" ~/basetrade/files/IndicatorInfo/lead_lag_ind_${1}.txt
echo "Log file is " ${tfile}_res_${1}
