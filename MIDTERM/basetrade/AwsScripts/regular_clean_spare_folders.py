#!/usr/bin/env python
import os
import sys
import subprocess

SPARE_FOLDERS_FILE = "/home/dvctrader/basetrade/AwsScripts/spare_folders.txt"
USER_SPARE_FOLDERS_FILE = "/home/dvctrader/basetrade/AwsScripts/user_spare_folders.txt"

SPARE_FOLDERS = ["/spare/local", "/spare0/local", "/spare1/local", "/spare2/local", "/spare3/local"]


def exec_function(prog):
    process = subprocess.Popen(prog, stdout=subprocess.PIPE, stdin=subprocess.PIPE,
                               stderr=subprocess.STDOUT, shell=True)
    (output, err) = process.communicate()
    ret = process.wait()
    if ret != 0:
        print "Command: " + prog + "\nStdout: " + output.strip() + " ,Stderr: " + str(err)
    return [output, err, ret]


def process_line(line):
    line = line.partition('#')[0].strip()
    return line


def get_folder(line):
    folder = line.split()[0]
    return folder


def get_options(line):
    options = ""
    line_split = line.split()
    if len(line_split) > 1:
        options = " ".join(line_split[1:])
    return options


def get_users():
    return exec_function('ls /media/user-accounts/')[0].strip().split('\n')


with open(SPARE_FOLDERS_FILE, 'r') as f:
    for line in f:
        line = process_line(line)
        if line != "":
            folder = get_folder(line)
            options = get_options(line)
            for base_folder in SPARE_FOLDERS:
                folder_path = base_folder + "/" + folder
                if os.path.isdir(folder_path) and os.listdir(folder_path) != []:
                    if options == "":
                        command = "find " + folder_path + "/* -atime +2 -delete"
                    else:
                        command = "find " + folder_path + "/* " + options + " -delete"
                    print command
                    exec_function(command)

USERS = get_users()
with open(USER_SPARE_FOLDERS_FILE, 'r') as f:
    for line in f:
        line = process_line(line)
        if line != "":
            folder = get_folder(line)
            options = get_options(line)
            for base_folder in SPARE_FOLDERS:
                for user in USERS:
                    folder_path = base_folder + "/" + user + "/" + folder
                    command = ''
                    if os.path.isdir(folder_path) and os.listdir(folder_path) != []:
                        if options == "":
                            command = "find " + folder_path + "/* -atime +2 -delete"
                        else:
                            command = "find " + folder_path + "/* " + options + " -delete"
                        print command
                        exec_function(command)
