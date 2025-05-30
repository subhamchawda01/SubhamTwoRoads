import sys
import subprocess

def exec_function(prog):
    process = subprocess.Popen(prog, stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    (output, err) = process.communicate()
    ret = process.wait()
    if ret != 0:
        raise ValueError("Program: " + prog + "\nStdout: " + output.strip() + " ,Stderr: " + str(err))
    return [output, err, ret]
