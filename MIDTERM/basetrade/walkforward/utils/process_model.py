import subprocess
import warnings
import os
import sys
import numpy as np
import pandas as pd

from walkforward.definitions import execs
from walkforward.analyse_type6_config import get_sumvars


def process_model(model_process_string, dates_file, start_time, end_time, processed_model_file, ilist, reg_string, tick_change_ = 1.0):
    """
    
    :param model_process_string: 
    :param dates_file: 
    :param start_time: 
    :param end_time: 
    :param processed_model_file: 
    :param ilist: 
    :param tick_change_: 
    :return:
     
     Here tick_change is taken as a param so that in cases where there is change in min_price_increment we can use the \
     same config across all the period without changing the param just by multiplying the model_stdev with \
     (min_price_increment on refresh date)/(min_price_increment today); so model_stdev should be given as the model_stdev\
      you would want on today's date
    """
    temp_processed_model_file = processed_model_file + "_scaled"
    model_process_tgt_stdev = 1
    model_process_args = model_process_string.split()
    model_process_algo = int(model_process_args[0])

    if model_process_algo == 1:
        if len(model_process_args) > 1:
            model_process_tgt_stdev = str(float(model_process_args[1])*tick_change_)
        else:
            raise ValueError("model tgt stdev missing for model_process_algo: " + model_process_algo)

    print(model_process_algo)
    print(model_process_tgt_stdev)

    if model_process_algo == 0:
        return
    elif model_process_algo == 1:
        work_dir = os.path.dirname(processed_model_file)
        regdata_file = os.path.join(work_dir, "filtered_rdata_file")
        data = np.loadtxt(regdata_file)
        data = data[:,1:]
        indicators_list = []
        weight_map = {}

        ilist_lines = open(ilist,'r').read().splitlines()
        for line in ilist_lines:
            tokens = line.strip().split()
            if len(tokens) > 0 and tokens[0] == "INDICATOR":
                end_indx = len(tokens)
                for j in range(len(tokens)):
                    if tokens[j][0] == '#':
                        end_indx = j
                        break
                indicator = ' '.join(x for x in tokens[2:end_indx])
                # print (indicator)
                weight_map[indicator] = '0'
                indicators_list.append(indicator)
        mfh = open(processed_model_file, 'r')

        for line in mfh:
            tokens = line.strip().split()
            if len(tokens) > 0 and tokens[0] == "INDICATOR":
                end_indx = len(tokens)
                for j in range(len(tokens)):
                    if tokens[j][0] == '#':
                        end_indx = j
                        break
                indicator = " ".join(x for x in tokens[2:end_indx])
                if indicator in weight_map:
                    weight_map[indicator] = str(tokens[1])
                else:
                    raise ValueError(indicator + ": this indicator KEY is missing in init_model weights map !! did you"
                                                 " change ilist while process is running ?")
        mfh.close()
        coeffs = []
        for indicator in indicators_list:
            coeffs.append(weight_map[indicator])
        sum_vars = get_sumvars(pd.DataFrame(data=data), coeffs, reg_string)
        new_stdev = np.std(sum_vars)

        print("scaling the model to : " + model_process_tgt_stdev + "\n")
        scale = float(model_process_tgt_stdev) / float(new_stdev) 

        rescale_model_cmd = [execs.execs().rescale_model, processed_model_file, temp_processed_model_file, str(scale)]
        print(rescale_model_cmd)
        process = subprocess.Popen(' '.join(rescale_model_cmd), shell=True,
                                   stderr=subprocess.PIPE,
                                   stdout=subprocess.PIPE)
        out, err = process.communicate()
        if out is not None:
            out = out.decode('utf-8')
        if err is not None:
            err = err.decode('utf-8')

        errcode = process.returncode

        if len(err) > 0:
            warnings.warn("error in rescale_model " + err)
            warnings.warn("not scaling")
            return

        os.system("mv " + temp_processed_model_file + " " + processed_model_file)

    elif model_process_algo == 2:
        raise NotImplementedError
    else:
        raise NotImplementedError

    return
