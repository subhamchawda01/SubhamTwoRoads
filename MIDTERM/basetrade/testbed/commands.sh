for name in $* ;
do
    if [ -e reg_res_0.02_0_0_0.90_40_N ] ; then
	echo $HOME/basetrade/ModelScripts/predict_reg_data.py $name reg_res_0.02_0_0_0.90_40_N yhat-reg_res_0.02_0_0_0.90_40_N ;
	$HOME/basetrade/ModelScripts/predict_reg_data.py $name reg_res_0.02_0_0_0.90_40_N yhat-reg_res_0.02_0_0_0.90_40_N ;
	$HOME/basetrade/ModelScripts/get_mse.py yhat-reg_res_0.02_0_0_0.90_40_N ;
    fi
    echo $HOME/basetrade_install/bin/callFSLR $name 0.02 0 0 0.90 reg_res_0.02_0_0_0.90_40_N 40 INVALIDFILE ;
    $HOME/basetrade_install/bin/callFSLR $name 0.02 0 0 0.90 reg_res_0.02_0_0_0.90_40_N 40 INVALIDFILE 2>/dev/null ;
done

for name in $* ;
do
    if [ -e reg_res_0.02_0_0_0.90_40_H ] ; then
	echo $HOME/basetrade/ModelScripts/predict_reg_data.py $name reg_res_0.02_0_0_0.90_40_H yhat-reg_res_0.02_0_0_0.90_40_H ;
	$HOME/basetrade/ModelScripts/predict_reg_data.py $name reg_res_0.02_0_0_0.90_40_H yhat-reg_res_0.02_0_0_0.90_40_H ;
	$HOME/basetrade/ModelScripts/get_mse.py yhat-reg_res_0.02_0_0_0.90_40_H ;
    fi
    echo $HOME/basetrade_install/bin/callFSLR $name 0.02 0 0 0.90 reg_res_0.02_0_0_0.90_40_H 40 INVALIDFILE ilist_BR_DOL_0_US_OMix_OMix_fsg1.J0_r ;
    $HOME/basetrade_install/bin/callFSLR $name 0.02 0 0 0.90 reg_res_0.02_0_0_0.90_40_H 40 INVALIDFILE ilist_BR_DOL_0_US_OMix_OMix_fsg1.J0_r 2>/dev/null ;
done

