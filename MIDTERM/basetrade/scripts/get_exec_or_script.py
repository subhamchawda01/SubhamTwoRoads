#!/usr/bin/env python
"""
Script to find the location of an exec or a script.

Suppose I want to call sim_strategy, I should be able to call this function with a list of additional paths to get correct path for sim strategy

Right now directories in default path for execs are

$HOME/LiveExec/bin/
$PATH
$HOME/cvquant_install/basetrade/bin

$HOME/LiveExec/scripts/
$HOME/LiveExec/ModelScripts/
$PATH

"""
import os
import sys
from os.path import expanduser
import argparse
sys.path.append(os.path.expanduser('~/basetrade/'))
from walkforward.utils.search_exec_or_script import search_script, search_exec


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', dest='script_name', help="script name", type=str)
    parser.add_argument('-e', dest='exec_name', help="binary name", type=str)
    args = parser.parse_args()

    if args.script_name is not None:
        print((search_script(args.script_name, list_of_additional_paths=[], search_deeper=False)))
    else:
        pass

    if args.exec_name is not None:
        print((search_exec(args.exec_name, list_of_additional_paths=[], search_deeper=False)))
    else:
        pass
