#!/usr/bin/env
import os
import sys
import subprocess

from walkforward.definitions.execs import paths
from walkforward.utils.search_exec_or_script import search_exec
from walkforward.utils.run_exec import exec_function


def get_shortcodes():
    return ["HHI_0", "XT_0", "VX_0"]


def get_all_shortcodes():
    directory = paths().modelling + '/strats/'
    dirs = next(os.walk(directory))[1]
    return list(set(dirs).intersection(set(get_valid_shortcodes())))


def get_valid_shortcodes():
    [output, err, ret] = exec_function(search_exec('get_contract_specs') +
                                       ' ALL 20160711 EXCHANGE | cut -d" " -f1 | sort | uniq')
    return output.split('\n')
