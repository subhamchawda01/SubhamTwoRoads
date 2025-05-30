#!/bin/bash

tfile=$HOME/strat_counts.txt;
rm -f $tfile;

for SHC in BR_DOL_0 BR_IND_0 BR_WIN_0 DI1F16 DI1F15 DI1F19 DI1F17 DI1N14 DI1F18 DI1F21 \
ZT_0 ZF_0 ZN_0 ZB_0 UB_0 \
FGBS_0 FGBM_0 FGBL_0 FESX_0 FDAX_0 FOAT_0 FOAM_0 FSMI_0 FBTP_0 FBTS_0 FESB_0 \
CGB_0 SXF_0 BAX_0 BAX_1 BAX_2 BAX_3 BAX_4 BAX_5 BAX_6 \
KFFTI_0 JFFCE_0 LFZ_0 LFR_0 LFI_0 LFI_1 LFI_2 LFI_3 LFI_4 LFI_5 LFI_6 LFL_0 LFL_1 LFL_2 LFL_3 LFL_4 LFL_4 LFL_5 LFL_6 YFEBM_0 YFEBM_1 YFEBM_2 \
HHI_0 HSI_0 MHI_0 \
NK_0 NKM_0 NKM_1 ;
do
numstrats=`ls -l $HOME/modelling/strats/$SHC/*/* 2>/dev/null | grep -v total | wc -l`;
echo $SHC $numstrats >> $tfile;
done 

sort -rg -k2 $tfile > $tfile".tmp"; 
echo "Sorted list of number of strategies in pool per shortcode" > $tfile;

cat $tfile".tmp" > $tfile;

$HOME/infracore/scripts/mail_file_to.sh STRAT_COUNTS nseall@tworoads.co.in $tfile;
rm -f $tfile;
