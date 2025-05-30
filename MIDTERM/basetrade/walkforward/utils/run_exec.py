#!/usr/bin/python

"""
Util to run exec

"""
import sys
import subprocess


def exec_function(prog):
    """
    Using 2.6 syntax for now
    Handling for None in case of output or err
    :param prog: 
    :return:

    """
    process = subprocess.Popen(prog, stdout=subprocess.PIPE, stdin=subprocess.PIPE,
                               stderr=subprocess.STDOUT, shell=True)
    (output, err) = process.communicate()
    ret = process.wait()
    if err != None:
        err = err.decode('utf-8')
    else:
        err = "None"
    if output != None:
        output = output.decode('utf-8')
    else:
        output = "None"
    if ret != 0:
        print("Program: " + prog + "\nStdout: " + output.strip() + " ,Stderr: " + str(err))
        sys.exit(1)
    return [output, err, ret]
