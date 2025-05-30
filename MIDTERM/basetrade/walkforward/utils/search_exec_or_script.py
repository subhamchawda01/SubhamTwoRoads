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


def usage():
    print(
        'Script to find the location of an exec or a script. Suppose I want to call sim_strategy, I should be able to call this function with a list of additional paths to get correct path for sim strategy. Example: search_exec_or_script.py -f "sim_strategy", or in python search_exec_or_script("sim_strategy",[path1,path2])')


def is_non_zero_file(fpath):
    """checks if file is there and is non zero"""
    return os.path.isfile(fpath) and os.path.getsize(fpath) > 0


def get_file_in_nested_dir(dirname, filename):
    """ get a file that may be buried deep inside a nested folder structure"""
    for subdir, dirs, files in os.walk(dirname):
        if filename in files:
            fullname = subdir + """/""" + filename
            if is_non_zero_file(fullname):
                return [True, fullname]
    return [False, None]


def get_file_in_directory(dirname, filename):
    """ get a file if it is in that directory"""
    fullname = dirname + """/""" + filename
    if is_non_zero_file(fullname):
        return [True, fullname]
    return [False, None]


def search_exec(exec_name, list_of_additional_paths=[], search_deeper=False):
    """main function that takes an exec name (exact name) and list of additional paths to look in addition to $PATH variable"""

    # This path is used in Jenkins
    if os.environ.get('DEPS_INSTALL'):
        JENKINS_BIN_DIR = os.environ['DEPS_INSTALL'] + '/basetrade/bin'
        list_of_additional_paths.append(JENKINS_BIN_DIR)

    HOME_DIR = expanduser("~")
    LIVE_BIN_DIR = HOME_DIR + '/LiveExec/bin'
    BASETRADE_BIN_DIR = HOME_DIR + '/cvquant_install/basetrade/bin'
    list_of_additional_paths.append(LIVE_BIN_DIR)
    list_of_additional_paths.append(BASETRADE_BIN_DIR)
    # print(list_of_additional_paths)
    # Take default exec search path from SHELL
    paths = os.environ['PATH'].split(':')
    list_of_additional_paths.extend(paths)

    # First search in top level
    for path in list_of_additional_paths:
        [result, file] = get_file_in_directory(path, exec_name)
        if result:
            return file

    if search_deeper:
        # Then search deeper if we need to
        for path in list_of_additional_paths:
            [result, file] = get_file_in_nested_dir(path, exec_name)
            if result:
                return file

    # I don't like print statements to stdout. If we want to send
    # and error, send an error code.

    # Removing exit since it is causing many scripts to fail, as it doesn't create problems since wherever the script is reference will throw an error irrespectively
    #print("Could not find the exec requested anywhere, quitting.")
    # sys.exit(0)


def search_script(script_name, list_of_additional_paths=[], search_deeper=False):
    """main function that takes an exec name (exact name) and list of additional paths to look in addition to $PATH variable"""

    # This path is used in Jenkins
    if os.environ.get('DEPS_INSTALL'):
        JENKINS_SCRIPTS_DIR = os.environ['DEPS_INSTALL'] + '/basetrade/scripts'
        JENKINS_MODELSCRIPTS_DIR = os.environ['DEPS_INSTALL'] + '/basetrade/ModelScripts'
        JENKINS_ROBUSTAPI_DIR = os.environ['DEPS_INSTALL'] + '/basetrade/RobustnessCheckApi'
        list_of_additional_paths.append(JENKINS_SCRIPTS_DIR)
        list_of_additional_paths.append(JENKINS_MODELSCRIPTS_DIR)
        list_of_additional_paths.append(JENKINS_ROBUSTAPI_DIR)

    HOME_DIR = expanduser("~")
    LIVE_SCRIPTS_DIR = HOME_DIR + '/LiveExec/scripts'
    BASETRADE_SCRIPTS_DIR = HOME_DIR + '/cvquant_install/basetrade/scripts'

    list_of_additional_paths.append(LIVE_SCRIPTS_DIR)
    list_of_additional_paths.append(BASETRADE_SCRIPTS_DIR)

    LIVE_MODELSCRIPTS_DIR = HOME_DIR + '/LiveExec/ModelScripts'
    BASETRADE_MODELSCRIPTS_DIR = HOME_DIR + '/cvquant_install/basetrade/ModelScripts'
    BASETRADE_ROBUSTAPI_DIR = HOME_DIR + '/cvquant_install/basetrade/RobustnessCheckApi'

    list_of_additional_paths.append(LIVE_MODELSCRIPTS_DIR)
    list_of_additional_paths.append(BASETRADE_MODELSCRIPTS_DIR)
    list_of_additional_paths.append(BASETRADE_ROBUSTAPI_DIR)

    WALKFORWARD_SCRIPTS_DIR = HOME_DIR + '/cvquant_install/basetrade/walkforward'
    list_of_additional_paths.append(WALKFORWARD_SCRIPTS_DIR)

    # REPO = 'basetrade'
    # INSTALL_DIR = HOME_DIR + '/' + 'cvquant_install'
    # GENPERLLIB_DIR = HOME_DIR + '/' + REPO + '_install/GenPerlLib' # I don't htink we will need this
    # list_of_additional_paths.append(GENPERLLIB_DIR) # I don't think we load any executable scripts from here.

    # First search in top level
    for path in list_of_additional_paths:
        [result, file] = get_file_in_directory(path, script_name)
        if result:
            return file

    if search_deeper:
        # Then search deeper if we need to
        for path in list_of_additional_paths:
            [result, file] = get_file_in_nested_dir(path, script_name)
            if result:
                return file

    print("Could not find the exec requested anywhere, quitting.")
    sys.exit(0)
