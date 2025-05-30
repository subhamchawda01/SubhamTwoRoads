import pandas as pd
import os
import argparse
import sys


def process_row(row, column_list, base_name, random_file):
    i = 1
    row_data = row.values.tolist()
    if not os.path.exists(base_name + "/" + row_data[0]):
        os.makedirs(base_name + "/" + row_data[0])
    base_name = base_name + "/" + row_data[0] + "/"
    f = open(random_file, "w")
    for x in row_data[1:]:
        if x == "|":
            f.close()
            f = open(base_name + column_list[i], "w+")
        else:
            f.write(column_list[i] + " = " + x + "\n")
        i = i + 1


parser = argparse.ArgumentParser()
parser.add_argument('file_path', help='Input File path')
parser.add_argument('folder_path', help='Folder inside which configs will be made')

args = parser.parse_args()

if args.file_path:
    file_path = args.file_path
else:
    sys.exit('Please provide input file path')

if args.folder_path:
    folder_path = args.folder_path
else:
    sys.exit('Please provide input folder path')

data = pd.read_csv(file_path, header=None)
column_list_ = list(data.loc[0])
data = data.drop(data.index[[0]])
none_filename_ = folder_path + "/none"
random = data.apply(process_row, args=(column_list_, folder_path, none_filename_), axis=1)
os.remove(none_filename_)
