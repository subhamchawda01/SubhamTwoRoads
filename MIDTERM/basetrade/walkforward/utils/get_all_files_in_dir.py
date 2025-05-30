#!/usr/bin/env python

"""
Gets list of all files in a directory, interating through 2 levels
"""

import os
import sys


def get_all_files_in_dir(dirname, filelist):
    for subdir, dirs, files in os.walk(dirname):
        add_file_to_list(files, filelist)
    return filelist


def add_file_to_list(files, filelist):
    for file in files:
        if file not in filelist:
            filelist.append(file)
        else:
            print("Already added the file .." + file, file=sys.stderr)


def exists(name, path):
    for root, dirs, files in os.walk(path):
        if name in files:
            #print( os.path.join(root,name))
            return True
    return False
