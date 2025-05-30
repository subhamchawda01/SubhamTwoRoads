#!/usr/bin/env python
"""
for a given strat/model param, prints the complete path

"""
import os
import sys


def get_full_path(basename):
    modelling_dir = os.path.expanduser("~/modelling")
    modelling_dir = "/home/dvctrader/modelling"
    # search for strats first
    dirs_to_search = ['strats', 'wf_strats', 'wf_staged_strats', 'staged_strats', 'params', 'models']

    for contents in dirs_to_search:
        dirname = modelling_dir + '/' + contents
        shcdirs = [os.path.join(dirname, fn) for fn in next(os.walk(dirname))[1]]
        # directory containing shortcodes
        for shc_dir in shcdirs:
            is_ebt_dir = False
            if contents == 'params':
                for filename in next(os.walk(shc_dir))[2]:
                    if filename == basename:
                        return os.path.join(shc_dir, filename)
            else:
                # directory containing the timeperids/EBT
                for tp_dir in next(os.walk(shc_dir))[1]:
                    tp_full_dir = os.path.join(shc_dir, tp_dir)
                    if tp_dir == 'EBT':
                        # directory containing strats/models or EventName
                        for event_dir in [os.path.join(tp_full_dir, fn) for fn in next(os.walk(tp_full_dir))[1]]:
                            for strat in next(os.walk(event_dir))[2]:
                                if strat == basename:
                                    return os.path.join(event_dir, strat)
                    else:
                        for strat in next(os.walk(tp_full_dir))[2]:
                            if strat == basename:
                                return os.path.join(tp_full_dir, strat)


def get_all_files_in_dir(dirname):
    filelist = []
    for subdir, dirs, files in os.walk(dirname):
        filelist.extend(files)
    return filelist


def clean_parent_dir(work_dir_):
    # remove parent work_dir, everything except: stdout.txt stderr.txt files
    files_to_keep = ["stdout.txt", "stderr.txt"]
    dirs_to_keep = []
    for t_root, t_dirs, t_files in os.walk(work_dir_):
        for t_file in t_files:
            if t_file not in files_to_keep:
                os.remove(os.path.join(t_root, t_file))
        for t_dir in t_dirs:
            if t_dir not in dirs_to_keep:
                os.rmdir(os.path.join(t_root, t_dir))

                os.remove(os.path.join(t_root, t_file))
        for t_dir in t_dirs:
            if t_dir not in dirs_to_keep:
                os.rmdir(os.path.join(t_root, t_dir))
