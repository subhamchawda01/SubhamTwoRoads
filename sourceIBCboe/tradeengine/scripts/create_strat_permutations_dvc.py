import os
import pandas as pd
import shutil
import itertools
import argparse
import sys
import subprocess
from utils import get_list_of_dirs_given_pattern, get_list_of_files_given_pattern


def change_param_for_single_file(file_path, param_name_list, operator_list_, param_value_list_):
    try:
        file_data = pd.read_csv(file_path, sep="\s+", names=["Prefix","Name", "Value"])
    except Exception as e:
        return
    file_data.set_index("Name", inplace=True)
    i = 0
    for params in param_name_list:
        for param_name in params.split(","):
            if param_name in file_data.index:
                val = file_data.loc[param_name, 'Value']
                try:
                    val = float(val)
                except ValueError:
                    param_value_list_[i] = r'"' + param_value_list_[i] + r'"'
                expression_string = "val " + operator_list_[i] + " " + param_value_list_[i]
                loc = locals()
                exec (expression_string, {}, loc)
                file_data.loc[param_name, 'Value'] = str(loc['val'])
        i = i + 1
    file_data = file_data.reset_index()
    file_data[["Prefix","Name", "Value"]].to_csv(file_path, sep=" ", encoding='utf-8', header=False, index=None)


def make_strat_for_a_permutation(values_, data_, strat_file_, param_file_, out_folder_):
    data_['Values'] = values_
    dest_strat_name_ = out_folder_ + "/strats/Iter_" + '_'.join(values_) 
    dest_param_name_ = out_folder_ + "/params/Iter_" + '_'.join(values_) 

    try:
        os.remove(dest_strat_name_)
    except OSError:
        pass

    strat_data_ = subprocess.check_output(["cat",strat_file_]).split()
    strat_data_ = [x.rstrip().decode("utf-8") for x in strat_data_]
    strat_data_[4] = dest_param_name_
    strat_data_ = " ".join(strat_data_)

    with open(dest_strat_name_, "w") as text_file:
        text_file.write(strat_data_)

    shutil.copyfile(param_file_,dest_param_name_)
    change_param_for_single_file(dest_param_name_, data_['Params'].values.tolist(), data_['Operator'].values.tolist(), data_['Values'].values.tolist())


def create_strat_permutations(strat_file, out_folder, permutation_file):
    try:
        shutil.rmtree(out_folder)
    except OSError:
        pass

    if not os.path.exists(out_folder):
        os.makedirs(out_folder)

    out_folder = os.path.abspath(out_folder)

    dest_strat_folder_ = out_folder + "/strats/"
    if not os.path.exists(dest_strat_folder_):
        os.makedirs(dest_strat_folder_)

    dest_param_folder_ = out_folder + "/params/"
    if not os.path.exists(dest_param_folder_):
        os.makedirs(dest_param_folder_)

    param_file = subprocess.check_output(["cat",strat_file]).split()[4].rstrip().decode("utf-8")

    data = pd.read_csv(permutation_file, names=["Params", "Operator", "Values"], delim_whitespace=True)
    data['Values'] = data['Values'].astype('str');
    values_permutations_ = [values.split(',') for values in data['Values'].values.tolist()]
    values_lists_ = [make_strat_for_a_permutation(list(s), data, strat_file, param_file, out_folder)
                     for s in itertools.product(*values_permutations_)]

if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument('strat_file', help='Strat folder to run permutations on')
    parser.add_argument('out_folder', help='Folder to store permutations')
    parser.add_argument('permutation_file', help='Permutation file')

    args = parser.parse_args()

    if args.strat_file:
        strat_file_ = args.strat_file
    else:
        sys.exit('Please provide input strat file')

    if args.out_folder:
        out_folder_ = args.out_folder
    else:
        sys.exit('Please provide folder to store permutations')

    if args.permutation_file:
        permutation_file_ = args.permutation_file
    else:
        sys.exit('Please provide permutation file')

    create_strat_permutations(strat_file_, out_folder_, permutation_file_)
