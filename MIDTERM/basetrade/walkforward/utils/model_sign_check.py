from walkforward.wf_db_utils.fetch_dump_model import fetch_modelid_from_modelname
from walkforward.wf_db_utils.fetch_dump_model import fetch_model_desc
import pandas as pd
import numpy as np

def check_sign_with_ilist(init_model, model_filename):
    modelid = fetch_modelid_from_modelname(init_model, False)
    model_desc = fetch_model_desc(modelid)
    if (not model_desc) or (modelid == -1):
        f = open(init_model, 'r')
        model_desc = f.read()
        f.close()
        model_lines = model_desc.splitlines()
    else:
        model_lines = model_desc.splitlines()

    # Constructing the initial signs of the ilist

    indicators_list = []
    weight_map = {}
    for line in model_lines:
        tokens = line.strip().split()
        if len(tokens) > 0 and tokens[0] == "INDICATOR":
            end_indx = len(tokens)
            for j in range(len(tokens)):
                if tokens[j][0] == '#':
                    end_indx = j
                    break
            indicator = ' '.join(x for x in tokens[2:end_indx])
            sign = 1
            # : indicates siglr and the number after the : is the beta, so taking the sign of that
            if ":" in tokens[1]:
                beta = tokens[1].split(':')[-1]
                if beta[0] == '-':
                    sign = -1
            elif tokens[1][0] == '-':
                sign = -1
            weight_map[indicator] = sign
            indicators_list.append(indicator)

    mfh = open(model_filename, 'r')

    # Reading the final model and checking if sign matches with intial sign of indicators
    table_entry = ""
    is_empty_model = True
    sign_match = True
    for line in mfh:
        tokens = line.strip().split()

        if len(tokens) > 0 and tokens[0] == "INDICATOR":
            end_indx = len(tokens)
            for j in range(len(tokens)):
                if tokens[j][0] == '#':
                    end_indx = j
                    break
            indicator = " ".join(x for x in tokens[2:end_indx])
            sign = 1
            if ":" in tokens[1]:
                beta = tokens[1].split(':')[-1]
                if beta[0] == '-':
                    sign = -1
            elif tokens[1][0] == '-':
                sign = -1
            if sign * weight_map[indicator] < 0:
                sign_match = False
            weight_map[indicator] = (tokens[1])
            is_empty_model = False

    mfh.close()

    if is_empty_model:
        return False
    if sign_match:
        return True
    else:
        return False

def get_sign_list(ilist_):
    weights_ =[]
    mfh = open(ilist_, 'r')

    for line in mfh:
        tokens = line.strip().split()

        if len(tokens) > 0 and tokens[0] == "INDICATOR":
            sign = 1

            if tokens[1][0] == '-':
                sign = -1

            weights_.append(sign)

    mfh.close()
    return weights_

def make_sign_as_ilist_for_nnls(ilist_, regdata_filename_):
    weights_ = get_sign_list(ilist_)
    weights_.insert(0,1)

    df = pd.read_csv(regdata_filename_, header = None, sep = " ")
    df = df*weights_
    df.to_csv(regdata_filename_,sep = " ",header = False, index = False)

def make_ind_sign_as_ilist_for_nnls(ilist_, model_):
    ilistfh = open(ilist_,'r')
    weight_map_orig = {}
    for line in ilistfh:
        tokens = line.strip().split()
        if len(tokens) > 0 and tokens[0] == "INDICATOR":
            end_indx = len(tokens)
            for j in range(len(tokens)):
                if tokens[j][0] == '#':
                    end_indx = j
                    break
            indicator = ' '.join(x for x in tokens[2:end_indx])
            weight_map_orig[indicator] = np.sign(float(tokens[1]))
    ilistfh.close()

    line_to_write = ""
    regfh = open(model_,'r')
    weight_map_new = {}
    for line in regfh:
        tokens = line.strip().split()
        if len(tokens) > 0 and tokens[0] == "INDICATOR":
            end_indx = len(tokens)
            for j in range(len(tokens)):
                if tokens[j][0] == '#':
                    end_indx = j
                    break
            indicator = ' '.join(x for x in tokens[2:end_indx])
            tokens[1] = str(weight_map_orig[indicator]*float(tokens[1]))
            line_ = " ".join(tokens)
            line_to_write += line_ + "\n"
        else:
            line_to_write += line
    regfh.close()


    rofhw = open(model_, 'w')
    rofhw.write(line_to_write)
    rofhw.close()


