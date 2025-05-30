import os
import sys
import subprocess
from .support import get_shortcodes


def exec_func(prog):
    process = subprocess.Popen(prog, stdout=subprocess.PIPE, stdin=subprocess.PIPE,
                               stderr=subprocess.STDOUT, shell=True)
    (output, err) = process.communicate()
    ret = process.wait()
    return [output, err, ret]


def get_strats(shortcode, num_strats):
    [output, err, ret] = exec_func('find /home/dvctrader/modelling/strats/' +
                                   shortcode + ' -type f | sort -R |tail -' + str(num_strats))
    output = output.strip()
    return output.split()


def run_sim_strategy(sim_strategy_exec, strat):
    print(sim_strategy_exec + " SIM " + strat + " 809 20160701 | grep SIMRESULT")
    [output, err, ret] = exec_func(sim_strategy_exec + " SIM " + strat + " 809 20160701 | grep SIMRESULT")
    print(output, err, ret)
    output = output.strip()
    output = output.splitlines()[-1]
    return output


def is_valid_result(sim_result, valid_sim_result):
    print("Sim Result: ", sim_result)
    print("Valid Sim Result: ", valid_sim_result)
    sim = sim_result.split()
    valid_sim = valid_sim_result.split()
    pnl = float(sim[1])
    valid_pnl = float(valid_sim[1])
    vol = float(sim[2])
    valid_vol = float(sim[2])
    pnl_diff = 0
    vol_diff = 0
    if valid_pnl != 0:
        pnl_diff = abs((pnl - valid_pnl) / valid_pnl)
    if valid_vol != 0:
        vol_diff = abs((vol - valid_vol) / valid_vol)
    return pnl_diff < 0.2 and vol_diff < 0.2


def test_run_for_all():
    SIM_STRATEGY_EXEC = os.getenv('SIM_STRATEGY_EXEC')
    num_strats = 2
    strats = []
    sim_results = []
    shortcodes = get_shortcodes()
    total = 0
    valid = 0
    for shc in shortcodes:
        strats = get_strats(shc, num_strats)
        for strat in strats:
            sim_result = run_sim_strategy(SIM_STRATEGY_EXEC, strat)
            valid_sim_result = run_sim_strategy('/home/dvctrader/basetrade_install/bin/sim_strategy', strat)
            total = total + 1
            if is_valid_result(sim_result, valid_sim_result):
                valid = valid + 1
            else:
                print(strat, sim_result, valid_sim_result)
    assert (total == valid)
