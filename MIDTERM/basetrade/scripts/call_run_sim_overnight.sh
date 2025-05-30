if [ -e $HOME/.gcc_profile ] ; 
then 
    source $HOME/.gcc_profile ;
else
    export NEW_GCC_LIB=/usr/local/lib
    export NEW_GCC_LIB64=/usr/local/lib64
    export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH
fi

#if [ ! -e $HOME/locks/call_run_sim_overnight.lock ] ; then
#touch $HOME/locks/call_run_sim_overnight.lock ;

CRSOP_SCRIPT=$HOME/LiveExec/scripts/call_run_sim_overnight_perdir.pl ;
if [ ! -e $CRSOP_SCRIPT ] ; then 
CRSOP_SCRIPT=$HOME/basetrade_install/scripts/call_run_sim_overnight_perdir.pl ;
fi

for prod in KFFTI_0 JFFCE_0 LFZ_0 LFR_0 FESX_0 FGBS_0 FGBM_0 FGBL_0 FGBX_0 FDAX_0 ZF_0 ZN_0 ZB_0 UB_0 CGB_0 BAX_0 BAX_1 BAX_2 BAX_3 BAX_4 BAX_5 BAX_6 BR_DOL_0 BR_IND_0 BR_WIN_0 
do
    $CRSOP_SCRIPT $prod;
done


#rm -f $HOME/locks/call_run_sim_overnight.lock ;
#else
#   echo "$HOME/locks/call_run_sim_overnight.lock present";
#fi
