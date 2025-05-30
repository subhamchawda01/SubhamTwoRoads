import os
from scripts.create_json_string import create_json_string
import sys
import subprocess
import datetime

sys.path.append(os.path.expanduser('~/basetrade/'))
sys.path.append(os.path.expanduser('~/grid/'))
from grid.client.api import *
from walkforward.definitions import execs


def run_sim(work_dir, shortcode, run_sim_dates, logfilename, using_grid):
    '''

    Just runs simulation on the on the strategies present in the strats folder in the work directory. It also takes additional
    arguments to control the output of the run simulation computation.



    work_dir: str
                The full path of the pnl modelling work directory
    shortcode: str
                The product shortcode whose pnl model is to be run

    start_date: str
                The start date from which run simulation is to be run

    end_date:  str
                The end date till which run simulation is to be run

    logfilename: str
                The full path of the main log file .

    skip_days_file: str
                The full path of the skip days file

    use_days_file:str
                The full path of the specefic days file on which to run the simulation on.

    using_grid: Boolean
                The flag to control whether to run_simulation using grid or not.


    return:None


    '''

    strats_dir = os.path.join(work_dir, "strats_dir")
    results_dir = os.path.join(work_dir, "local_results_base_dir")

    # write the dates to run simulation on in a file, this file is later passed to the run simulation command.

    run_sim_dates_file = os.path.join(work_dir, "run_sim_dates")
    w = open(run_sim_dates_file, 'w')
    for dt in run_sim_dates:
        w.write("%s\n" % dt)
    w.close()
    run_sim_dates.sort()
    start_date = str(run_sim_dates[0])
    end_date = str(run_sim_dates[-1])
    # run simulation
    if not using_grid:
        run_simulation_cmd = [execs.execs().run_simulations, shortcode,
                              strats_dir, start_date, end_date, results_dir, "-dtlist", run_sim_dates_file, "--nogrid"]
    else:
        run_simulation_cmd = [execs.execs().run_simulations, shortcode,
                              strats_dir, start_date, end_date, results_dir, "-dtlist", run_sim_dates_file, "--grid"]

    logfile = open(logfilename, 'a')
    logfile.write("RUN SIMULATIONS\n")
    logfile.write(' '.join(run_simulation_cmd))
    logfile.write("\n")

    print(' '.join(run_simulation_cmd))

    logfile.write("\nRunSimulation Start Time : " + str(datetime.datetime.now()) + "\n")
    logfile.flush()
    errcode = subprocess.call(' '.join(run_simulation_cmd), shell=True)

    if int(errcode) != 0:
        logfile.write("Exiting because run_simulations ended with return code " + str(errcode) + "\n")
        sys.exit(1)

    logfile.write("\nRunSimulation End Time : " + str(datetime.datetime.now()) + "\n")
    logfile.write("RUN SIMULATIONS COMPLETE\n")
    logfile.write("-------------------------------------\n")
    logfile.flush()
    logfile.close()


def run_sim_for_selected_strat(work_dir, shortcode, start_date, end_date, testing_days_file, logfilename, using_grid):
    strats_dir = os.path.join(work_dir, "selected_strat")
    results_dir = os.path.join(work_dir, "local_results_base_dir")

    # write the dates to run simulation on in a file, this file is later passed to the run simulation command.
    if not using_grid:
        run_simulation_cmd = [execs.execs().run_simulations, shortcode,
                              strats_dir, start_date, end_date, results_dir, "-dtlist", testing_days_file, "--nogrid"]
    else:
        run_simulation_cmd = [execs.execs().run_simulations, shortcode,
                              strats_dir, start_date, end_date, results_dir, "-dtlist", testing_days_file, "--grid"]

    logfile = open(logfilename, 'a')
    logfile.write("RUN SIMULATIONS for selected Strat on test period\n")
    logfile.write(' '.join(run_simulation_cmd))
    logfile.write("\n")

    print(' '.join(run_simulation_cmd))

    logfile.write("\nRunSimulationTest Start Time : " + str(datetime.datetime.now()) + "\n")
    logfile.flush()
    errcode = subprocess.call(' '.join(run_simulation_cmd), shell=True)

    if int(errcode) != 0:
        logfile.write("Exiting because run_simulations ended with return code " + str(errcode) + "\n")
        sys.exit(1)

    logfile.write("\nRunSimulationTest End Time : " + str(datetime.datetime.now()) + "\n")
    logfile.write("RUN SIMULATIONS COMPLETE\n")
    logfile.write("-------------------------------------\n")
    logfile.flush()
    logfile.close()
