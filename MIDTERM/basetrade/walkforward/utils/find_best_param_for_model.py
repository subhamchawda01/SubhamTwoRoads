import os
import subprocess
import getpass
import time

from walkforward.choose_best_param_for_model import summarize_results_and_cloose
from walkforward.definitions import execs


def create_strats(work_dir, model_file_list, shortcode, execlogic, start_time, end_time, param_file_list, event_token):
    strats_dir = os.path.join(work_dir, "strats")
    models_dir = os.path.join(work_dir, "models_dir")
    params_dir = os.path.join(work_dir, "params_dir")
    query_id = "2222"

    os.system('mkdir -p ' + str(strats_dir))
    os.system('mkdir -p ' + str(models_dir))
    os.system('mkdir -p ' + str(params_dir))

    model_indx = 0
    for model in model_file_list:
        os.system("cp " + model + " " + os.path.join(models_dir, "model_" + str(model_indx)))
        model_indx += 1

    param_indx = 0
    for param in param_file_list:
        os.system("cp " + param + " " + os.path.join(params_dir, "param_" + str(param_indx)))
        param_indx += 1

    for i in range(param_indx):
        for j in range(model_indx):
            model_file = os.path.join(models_dir, "model_" + str(j))
            param_file = os.path.join(params_dir, "param_" + str(i))
            w = open(os.path.join(strats_dir, "strat_" + str(j) + "_" + str(i)), 'w')
            w.write("STRATEGYLINE " + shortcode + " " + execlogic + " " + model_file + " " + param_file + " " +
                    start_time + " " + end_time + " " + query_id + " " + event_token)
            w.close()


def find_best_param_for_model(model_file_list, param_file_list, config_json, start_date, end_date):
    if len(param_file_list) == 1 and len(model_file_list) == 1:
        return model_file_list[0], param_file_list[0]
    shortcode = config_json["shortcode"]
    start_time = config_json["start_time"]
    end_time = config_json["end_time"]
    exec_logic = config_json["execlogic"]
    event_token = config_json["event_token"]

    temp_work_dir = "/media/shared/ephemeral16/" + getpass.getuser() + "/pnl_modelling/" + shortcode + "/" + \
                    str(int(time.time() * 1000)) + "/"

    create_strats(temp_work_dir, model_file_list, shortcode, exec_logic,
                  start_time, end_time, param_file_list, event_token)

    results_dir = os.path.join(temp_work_dir, "local_results_base_dir")
    strats_dir = os.path.join(temp_work_dir, "strats")

    run_simulation_cmd = [execs.execs().run_simulations, shortcode,
                          strats_dir, str(start_date), str(end_date), results_dir]
    process = subprocess.Popen(' '.join(run_simulation_cmd), shell=True,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE)
    print(' '.join(run_simulation_cmd))
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')
    errcode = process.returncode
    print(out, err, errcode)
    time.sleep(90)
    logfilename = os.path.join(temp_work_dir, 'main_log_file.txt')
    logfile = open(logfilename, 'w')

    (model, param) = summarize_results_and_cloose(config_json, temp_work_dir, logfile, str(start_date),
                                                  str(end_date))

    model_indx = int(os.path.basename(model).split("_")[-1])
    param_indx = int(os.path.basename(param).split("_")[-1])

    print(model_file_list[model_indx], param_file_list[param_indx])
    return model_file_list[model_indx], param_file_list[param_indx]
