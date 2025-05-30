"""

Module to learn weights from given reg-data file

"""

import subprocess
import numpy as np

from walkforward.definitions import execs
from walkforward.utils.dump_content_to_file import write_model_to_file
## Commenting out following import as it will throw error untill
## '/media/shared/ephemeral16/dbhartiya/liblinear-weights-2.01/python' is recovered.
#from scripts.fill_based_model import get_fill_based_model
from pylib.pnl_modelling_utils.weights_util import *
from walkforward.utils.model_sign_check import make_sign_as_ilist_for_nnls, make_ind_sign_as_ilist_for_nnls


def call_ridge(ilist, regdata_filename, regress_exec_params, reg_output_file, final_model_filename):
    """
    calls R script
    :param ilist: 
    :param regdata_filename: 
    :param regress_exec_params: 
    :param reg_output_file: 
    :param final_model_filename: 
    :return: 
    """
    ridge_cmd = [execs.execs().ridge, regdata_filename, regress_exec_params, reg_output_file]
    process = subprocess.Popen(' '.join(ridge_cmd), shell=True,
                               stderr=subprocess.PIPE,
                               stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')
    errcode = process.returncode

    if len(err) > 0:
        raise ValueError("Error reported in Ridge call" + err)
        # if errcode != 0:
        #   raise ValueError("Error reported in Ridge call" + errcode)

    place_coeffs_cmd = [execs.execs().place_coeffs, final_model_filename, ilist, reg_output_file]
    process = subprocess.Popen(' '.join(place_coeffs_cmd), shell=True,
                               stderr=subprocess.PIPE,
                               stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')

    errcode = process.returncode
    print(err)
    print(out)

    if len(err) > 0:
        raise ValueError("Error reported in place_coeffs_cmd call" + err)
    # if errcode != 0:
    #    raise ValueError("Error reported in place_coeffs_cmd call" + errcode)
    print(err)
    print(out)
    return out


def call_siglr(ilist, regdata_filename, regress_exec_params, reg_output_file, final_model_filename):
    siglr_cmd = [execs.execs().siglr, regdata_filename, reg_output_file, regress_exec_params]
    process = subprocess.Popen(' '.join(siglr_cmd), shell=True,
                               stderr=subprocess.PIPE,
                               stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')

    errcode = process.returncode

    if len(err) > 0:
        raise ValueError("Error reported in SIGLR call" + err)
    # if errcode != 0:
    #    raise ValueError("Error reported in SIGLR call" + errcode)
    print(err)
    print(out)

    place_coeffs_cmd = [execs.execs().place_coeffs_siglr, final_model_filename, ilist, reg_output_file]
    process = subprocess.Popen(' '.join(place_coeffs_cmd), shell=True,
                               stderr=subprocess.PIPE,
                               stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')

    errcode = process.returncode
    if len(err) > 0:
        raise ValueError("Error reported in place_coeffs_cmd call" + err)
    # if errcode != 0:
    #    raise ValueError("Error reported in place_coeffs_cmd call" + errcode)
    print(err)
    print(out)
    return out


def call_fsrr(ilist, regdata_filename, regress_exec_params, reg_output_file, final_model_filename):
    tokens = regress_exec_params.split()
    if len(tokens) < 5:
        raise ValueError("min tokens 5 required for fsrr method, " + regress_exec_params)

    regularization_coeff = tokens[0]
    min_corr = tokens[1]
    first_indep_weight = tokens[2]
    must_include_first_k_independents = tokens[3]
    max_indep_corr = tokens[4]
    max_model_size = 12

    if len(tokens) > 5:
        max_model_size = tokens[5]

    match_icorrs = "N"
    if len(tokens) > 6:
        match_icorrs = tokens[6]

    fsrr_cmd = [execs.execs().fsrr, regdata_filename, regularization_coeff, min_corr, first_indep_weight,
                must_include_first_k_independents, max_indep_corr, reg_output_file, max_model_size,
                "INVALID"]  # avoid_high_sharpe_indep_check_index_filename_]
    print(fsrr_cmd)

    if match_icorrs == "I":
        fsrr_cmd = fsrr_cmd + [ilist]

    # set semantic sign check ignore-zeros
    semantic_sign = 'N'
    ignore_zeros = 'N'
    if len(tokens) > 7:
        semantic_sign = tokens[7]
        if len(tokens) > 8:
            ignore_zeros = tokens[8]

    fsrr_cmd.append(semantic_sign)
    fsrr_cmd.append(ignore_zeros)

    process = subprocess.Popen(' '.join(fsrr_cmd), shell=True,
                               stderr=subprocess.PIPE,
                               stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')

    # errcode = process.returncode
    if len(err) > 0:
        raise ValueError("Error reported in FSRR call" + err)

    print(err)
    print(out)

    place_coeffs_cmd = [execs.execs().place_coeffs, final_model_filename, ilist, reg_output_file]
    process = subprocess.Popen(' '.join(place_coeffs_cmd), shell=True,
                               stderr=subprocess.PIPE,
                               stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')

    # errcode = process.returncode
    if len(err) > 0:
        raise ValueError("Error reported in place_coeffs_cmd call" + err)

    # print(err)
    # print(out)
    return out


def call_lm(ilist, regdata_filename, regress_exec_params, reg_output_file, final_model_filename):
    """

    :param ilist: 
    :param regdata_filename: 
    :param regress_exec_params: 
    :param reg_output_file: 
    :param final_model_filename: 
    :return: 
    """
    # LM
    tokens = regress_exec_params.split()
    lm_cmd = [execs.execs().lm, regdata_filename, reg_output_file]
    print(lm_cmd)
    process = subprocess.Popen(' '.join(lm_cmd), shell=True,
                               stderr=subprocess.PIPE,
                               stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')

    # errcode = process.returncode

    if len(err) > 0:
        raise ValueError("Error reported in LM call" + err)
    print(err)
    print(out)

    place_coeffs_cmd = [execs.execs().place_coeffs_lasso, final_model_filename, ilist, reg_output_file]
    print(place_coeffs_cmd)
    process = subprocess.Popen(' '.join(place_coeffs_cmd), shell=True,
                               stderr=subprocess.PIPE,
                               stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')

    # errcode = process.returncode

    if len(err) > 0:
        raise ValueError("Error reported in place_coeffs_cmd call in LM" + err)
    # if errcode != 0:
    #    raise ValueError("Error reported in place_coeffs_cmd call" + errcode)

    print(err)
    print(out)
    return out


def call_nnls(ilist, regdata_filename, regress_exec_params, reg_output_file, final_model_filename):
    """
    # NNLS
    :param ilist: 
    :param regdata_filename: 
    :param regress_exec_params: 
    :param reg_output_file: 
    :param final_model_filename: 
    :return: 
    """

    tokens = regress_exec_params.split()
    if len(tokens) < 1:
        raise ValueError("min tokens 1 required for NNLS method, " + regress_exec_params)

    max_model_size = tokens[0]

    make_sign_as_ilist_for_nnls(ilist, regdata_filename)

    nnls_cmd = [execs.execs().nnls, regdata_filename, reg_output_file, max_model_size]
    print((' '.join(nnls_cmd)))
    process = subprocess.Popen(' '.join(nnls_cmd), shell=True,
                               stderr=subprocess.PIPE,
                               stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')

    # errcode = process.returncode

    if len(err) > 0:
        raise ValueError("Error reported in NNLS call" + err)

    print(err)
    print(out)

    place_coeffs_cmd = [execs.execs().place_coeffs_lasso, final_model_filename, ilist, reg_output_file]
    print((' '.join(place_coeffs_cmd)))

    process = subprocess.Popen(' '.join(place_coeffs_cmd), shell=True,
                               stderr=subprocess.PIPE,
                               stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')

    # errcode = process.returncode

    if len(err) > 0:
        raise ValueError("Error reported in place_coeffs_cmd call in NNLS" + err)
    make_ind_sign_as_ilist_for_nnls(ilist, final_model_filename)
    print(err)
    print(out)
    return out


def call_lasso(ilist, regdata_filename, regress_exec_params, reg_output_file, final_model_filename):
    """

    :param ilist: 
    :param regdata_filename: 
    :param regress_exec_params: 
    :param reg_output_file: 
    :param final_model_filename: 
    :return: 
    """

    param_tokens = regress_exec_params.split()
    max_model_size = 30
    if len(param_tokens) > 0:
        max_model_size = param_tokens[0]

    lasso_cmd = [execs.execs().lasso, regdata_filename, max_model_size, reg_output_file]

    process = subprocess.Popen(' '.join(lasso_cmd), shell=True,
                               stderr=subprocess.PIPE,
                               stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')

    if len(err) > 0:
        raise ValueError("Error reported in LM call" + err)

    place_coeffs_cmd = [execs.execs().place_coeffs_lasso, final_model_filename, ilist, reg_output_file]

    print((' '.join(place_coeffs_cmd)))

    process = subprocess.Popen(' '.join(place_coeffs_cmd), shell=True,
                               stderr=subprocess.PIPE,
                               stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')

    # errcode = process.returncode

    if len(err) > 0:
        raise ValueError("Error reported in place_coeffs_cmd call in LM" + err)

    # print(err)
    # print(out)
    return out


def call_slasso(ilist, regdata_filename, regress_exec_params, reg_output_file, final_model_filename):
    """

    :param ilist: 
    :param regdata_filename: 
    :param regress_exec_params: 
    :param reg_output_file: 
    :param final_model_filename: 
    :return: 
    """

    param_tokens = regress_exec_params.split()
    max_model_size = 30
    if len(param_tokens) > 0:
        max_model_size = param_tokens[0]

    lasso_cmd = [execs.execs().slasso, regdata_filename, max_model_size, reg_output_file]

    process = subprocess.Popen(' '.join(lasso_cmd), shell=True,
                               stderr=subprocess.PIPE,
                               stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')

    if len(err) > 0:
        raise ValueError("Error reported in LM call" + err)

    place_coeffs_cmd = [execs.execs().place_coeffs_lasso, final_model_filename, ilist, reg_output_file]

    print((' '.join(place_coeffs_cmd)))

    process = subprocess.Popen(' '.join(place_coeffs_cmd), shell=True,
                               stderr=subprocess.PIPE,
                               stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')

    # errcode = process.returncode

    if len(err) > 0:
        raise ValueError("Error reported in place_coeffs_cmd call in LM" + err)

    # print(err)
    # print(out)
    return out


def call_fsrmsh(ilist, regress_exec, regdata_filename, regress_exec_params, reg_output_file, final_model_filename):

    param_tokens = regress_exec_params.split()
    if len(param_tokens) < 5:
        ValueError("Min tokens required for fsrmsh is 5")

    # get all the params
    num_local_folds = param_tokens[0]
    min_correlation = param_tokens[1]
    first_indep_weight = param_tokens[2]
    must_include_first_k_indeps = param_tokens[3]
    max_indep_correlation = param_tokens[4]
    max_model_size = param_tokens[5]
    match_icors = param_tokens[6]

    basetrade_regress_exec = execs.execs().fsrmsh

    # select the exec here, rest of parameters are same for all these execs
    if regress_exec == 'FSRLM':
        basetrade_regress_exec = execs.execs().fsrlm
    elif regress_exec == 'FSRMFSS':
        basetrade_regress_exec = execs.execs().fsrmfss
    elif regress_exec == 'FSRSHRSQ':
        basetrade_regress_exec = execs.execs().fsrshrsq

    regress_exec_cmd = [basetrade_regress_exec, regdata_filename, num_local_folds, min_correlation,
                        first_indep_weight, must_include_first_k_indeps, max_indep_correlation, reg_output_file,
                        max_indep_correlation, max_model_size, "INVALID"]

    if match_icors == 'I':
        regress_exec_cmd.append(ilist)

    # probably for non-zero check
    if len(param_tokens) > 7:
        regress_exec_cmd.append(param_tokens[7])

    print((' '.join(regress_exec_cmd)))

    # run the regression
    process = subprocess.Popen(' '.join(regress_exec_cmd), shell=True,
                               stderr=subprocess.PIPE,
                               stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')

    # errcode = process.returncode

    if len(err) > 0:
        raise ValueError("Error reported in " + regress_exec + "call" + err)

    print(err)
    print(out)

    place_coeffs_cmd = [execs.execs().place_coeffs, final_model_filename, ilist, reg_output_file]
    process = subprocess.Popen(' '.join(place_coeffs_cmd), shell=True,
                               stderr=subprocess.PIPE,
                               stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')

    # errcode = process.returncode

    if len(err) > 0:
        raise ValueError("Error reported in place_coeffs_cmd call" + err)

    # print(err)
    # print(out)
    return out


def run_fill_based_model_generator(regress_exec_params, shortcode, ilist, param, date, num_days, start_time, end_time, datagen_args, skip_days_file, model_process_string, final_model_filename):
    """
    The function generates the model using fill based information. 
    :param regress_exec_params: example "FILL 1"
    :param shortcode: shortcode of the product
    :param ilist: ilist to be used for learning the model
    :param date: end date 
    :param num_days: number of days to be used in training
    :param start_time: trading start time
    :param end_time: end trading time
    :param datagen_args: datagen args "msecs l1events trades" 
    :param skip_days_file: skip days file to avoid generating data, default IF
    :param model_process_string: model_process_string as mentioned in type 6 config, used for scaling 
    :param final_model_filename: find model filename to store the trained model
    """
    datagen_args = datagen_args.split()
    msecs = datagen_args[0]
    events = datagen_args[1]
    trades = datagen_args[2]
    eco = "0"

    model_tgt_stdev = "0"
    model_process_args = model_process_string.split()
    model_process_algo = int(model_process_args[0])

    if model_process_algo == 0:
        model_tgt_stdev = "0"
    elif model_process_algo == 1:
        model_tgt_stdev = model_process_args[1]
    else:
        model_tgt_stdev = "0"

    regress_exec_params = regress_exec_params.split()
    use_l1_bias = regress_exec_params[1]

    get_fill_based_model(shortcode, ilist, param, date, num_days, start_time, end_time,
                         msecs, events, trades, eco,
                         skip_days_file, model_tgt_stdev, use_l1_bias, final_model_filename)


def call_corr_search(ilist, regdata_filename, regress_exec_params, final_model_filename, config_json):
    max_sum = int(regress_exec_params.split()[0])
    data = np.loadtxt(regdata_filename)
    stdev_indicators = np.std(data[:, 1:], axis=0)
    delta_y = data[:, 0]
    num_indicators = data.shape[1] - 1

    if "user_matrix" in config_json and config_json["user_matrix"] is not None:
        user_matrix = config_json["user_matrix"]
        print(user_matrix)
        weights = generate_weight_grid_from_matrix(0, num_indicators, user_matrix)
        weights = remove_all_zero_combination(weights)
    else:
        print("Indicators:", num_indicators, "Max sum:", max_sum)
        weights = generate_weight_grid(0, num_indicators, max_sum)
        weights = check_constraints_on_weights(weights, int(max_sum))
        weights = remove_similar_distance_weights(weights)
        weights = np.array(weights)
        enforce_sign_check(ilist, weights)
    new_weights = []
    for weight in weights:
        weight = np.divide(weight, stdev_indicators)
        new_weights.append(list(weight))
    new_weights = np.array(new_weights)
    weights, scaled_weights = remove_duplicate_weights(weights, new_weights)

    max_corr = 0
    selected_weights = []
    match_indx = -1
    indx = 0
    for weight in scaled_weights:
        sumvars = np.dot(data[:, 1:], weight)
        corr = np.corrcoef(delta_y, sumvars)[0][1]
        if corr > max_corr:
            max_corr = corr
            selected_weights = weight
            match_indx = indx
        indx += 1

    selected_weights = ",".join(list(map(str, selected_weights)))
    print("Max Correlation:", max_corr)
    print("Selected weights:", weights[match_indx, :])

    write_model_to_file(final_model_filename, ilist, selected_weights, False, open(ilist, 'r').read())

    return None


def run_method(ilist, regdata_filename, regress_exec_params, reg_output_file, final_model_filename, config_json=""):
    regress_exec = (regress_exec_params.split())[0]
    regress_exec_params = " ".join((regress_exec_params.split())[1:])

    if regress_exec == "RIDGE":
        out = call_ridge(ilist, regdata_filename, regress_exec_params, reg_output_file, final_model_filename)
    elif regress_exec == "SIGLR":
        out = call_siglr(ilist, regdata_filename, regress_exec_params, reg_output_file, final_model_filename)
    elif regress_exec == "FSRR":
        out = call_fsrr(ilist, regdata_filename, regress_exec_params, reg_output_file, final_model_filename)
    elif regress_exec == "LM":
        out = call_lm(ilist, regdata_filename, regress_exec_params, reg_output_file, final_model_filename)
    elif regress_exec == "NNLS":
        out = call_nnls(ilist, regdata_filename, regress_exec_params, reg_output_file, final_model_filename)
    elif regress_exec == 'FSRLM' \
            or regress_exec == 'FSRMFSS' \
            or regress_exec == 'FSRMSH' \
            or regress_exec == 'FSRSHRSQ':

        out = call_fsrmsh(ilist, regress_exec, regdata_filename, regress_exec_params,
                          reg_output_file, final_model_filename)
    elif regress_exec == "CORR":
        out = call_corr_search(ilist, regdata_filename, regress_exec_params, final_model_filename, config_json)
    elif regress_exec == "LASSO":
        out = call_lasso(ilist, regdata_filename, regress_exec_params, reg_output_file, final_model_filename)
    elif regress_exec == "SLASSO":
        out = call_slasso(ilist, regdata_filename, regress_exec_params, reg_output_file, final_model_filename)
    else:
        raise NotImplementedError

    # we can return bunch of things here, along with model file, given we have access to regfile
    # for starter,
    # per signal mean/stdev and cor
    # we can then further make the sum vars and return mean, stdev and cor, not easy for siglr :(

    return out
