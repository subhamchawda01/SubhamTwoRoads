

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


if [ ! -e $HOME/locks/call_run_sim_overnight.lock ] ; then
touch $HOME/locks/call_run_sim_overnight.lock ;

~/infracore/scripts/call_run_sim_overnight_perdir.pl FDAX_0
~/infracore/scripts/call_run_sim_overnight_perdir.pl FESX_0
~/infracore/scripts/call_run_sim_overnight_perdir.pl FGBS_0
~/infracore/scripts/call_run_sim_overnight_perdir.pl FGBM_0
~/infracore/scripts/call_run_sim_overnight_perdir.pl FGBL_0
#
~/infracore/scripts/call_run_sim_overnight_perdir.pl ES_0
~/infracore/scripts/call_run_sim_overnight_perdir.pl ZT_0
~/infracore/scripts/call_run_sim_overnight_perdir.pl ZF_0
~/infracore/scripts/call_run_sim_overnight_perdir.pl ZN_0
~/infracore/scripts/call_run_sim_overnight_perdir.pl ZB_0
~/infracore/scripts/call_run_sim_overnight_perdir.pl UB_0
#
~/infracore/scripts/call_run_sim_overnight_perdir.pl SXF_0
~/infracore/scripts/call_run_sim_overnight_perdir.pl CGB_0
#~/infracore/scripts/call_run_sim_overnight_perdir.pl BAX_0
~/infracore/scripts/call_run_sim_overnight_perdir.pl BAX_1
~/infracore/scripts/call_run_sim_overnight_perdir.pl BAX_2
~/infracore/scripts/call_run_sim_overnight_perdir.pl BAX_3
~/infracore/scripts/call_run_sim_overnight_perdir.pl BAX_4
~/infracore/scripts/call_run_sim_overnight_perdir.pl BAX_5
#~/infracore/scripts/call_run_sim_overnight_perdir.pl BAX_6

rm -f $HOME/locks/call_run_sim_overnight.lock ;
else
    echo "$HOME/locks/call_run_sim_overnight.lock present";
fi
