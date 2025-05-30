import os
import pandas as pd
import shutil
import itertools
import argparse
import sys
from utils import get_list_of_dirs_given_pattern, get_list_of_files_given_pattern


def copytree(src, dst, symlinks=False, ignore=None):
    for item in os.listdir(src):
        src_path = os.path.join(src, item)
        dest_path = os.path.join(dst, item)
        if os.path.isdir(src_path):
            shutil.copytree(src_path, dest_path, symlinks, ignore)
        else:
            shutil.copy2(src_path, dest_path)


def change_param_for_single_file(file_path, param_name_list, operator_list_, param_value_list_):
    try:
        file_data = pd.read_csv(file_path, sep="\s+", names=["Name", "Delim", "Value"])
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
    file_data.to_csv(file_path, sep=" ", encoding='utf-8', header=False)


def change_param_for_single_config_file(folder_list_, df, file_name, dest_folder_name_):
    [change_param_for_single_file(dest_folder_name_ + folder + "/" + file_name,
                                  df['Params'].values.tolist(), df['Operator'].values.tolist(),
                                  df['Values'].values.tolist()) for folder in folder_list_]
    [change_param_for_single_file(dest_folder_name_ + file_, ["LIVE_FOLDER"], ["="], [dest_folder_name_]) for file_ in
     get_list_of_files_given_pattern(dest_folder_name_, "LIVE")]


def make_strat_for_a_permutation(values_, data_, folder_list_, live_folder_, out_folder_):
    data_['Values'] = values_
    dest_folder_name_ = out_folder_ + "/Iter_" + '_'.join(values_) + "/"

    try:
        shutil.rmtree(dest_folder_name_)
    except OSError:
        pass
    if not os.path.exists(dest_folder_name_):
        os.makedirs(dest_folder_name_)
    copytree(live_folder_, dest_folder_name_)
    gb = data_.groupby('FileName')
    [change_param_for_single_config_file(folder_list_, x, key, dest_folder_name_) for key, x in gb]


def create_strat_permutations(strat_folder, expression, out_folder, permutation_file):
    try:
        shutil.rmtree(out_folder)
    except OSError:
        pass
    list_dir = get_list_of_dirs_given_pattern(strat_folder, expression)

    data = pd.read_csv(permutation_file, names=["FileName", "Params", "Operator", "Values"], delim_whitespace=True)
    data['Values'] = data['Values'].astype('str');
    values_permutations_ = [values.split(',') for values in data['Values'].values.tolist()]
    values_lists_ = [make_strat_for_a_permutation(list(s), data, list_dir, strat_folder, out_folder)
                     for s in itertools.product(*values_permutations_)]

if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument('strat_folder', help='Strat folder to run permutations on')
    parser.add_argument('expression', help='Pattern for folders to consider from strat folder')
    parser.add_argument('out_folder', help='Folder to store permutations')
    parser.add_argument('permutation_file', help='Permutation file')

    args = parser.parse_args()

    if args.strat_folder:
        strat_folder_ = args.strat_folder
    else:
        sys.exit('Please provide input strat folder')

    if args.expression:
        expression_ = args.expression
    else:
        sys.exit('Please provide pattern to choose folder from')

    if args.out_folder:
        out_folder_ = args.out_folder
    else:
        sys.exit('Please provide folder to store permutations')

    if args.permutation_file:
        permutation_file_ = args.permutation_file
    else:
        sys.exit('Please provide permutation file')

    create_strat_permutations(strat_folder_, expression_, out_folder_, permutation_file_)
