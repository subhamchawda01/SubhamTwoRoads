

"""
Module to create a strategy file from given generate strategy config file.
As of now, the functionality it has is to call generate_strategies and return the model_param files

We can call multiple processes simultaneously and wait for execution
"""

import sys
import os

import subprocess
#from multiprocessing.pool import worker


class create_strategy(object):
    generate_strategy = os.getenv("HOME") + '/basetrade_install/ModelScripts/generate_strategies.pl'
    summarize_local_results = os.getenv(
        "HOME") + '/basetrade_install/bin/summarize_local_results_dir_and_choose_by_algo'
    distributed_executor = '/home/dvctrader/dvccode/scripts/datainfra/celeryFiles/celeryClient/celeryScripts/client.py'

    config_to_outdirectory = {}
    config_to_model_param_content = {}
    worker_list = []
    config_to_strategy = {}

    def __init__(self):
        print("Creating the object")
        # get the list of workers
        worker_list_filename = '/mnt/sdf/JOBS/all_instances.txt'
        worker_list_file = open(worker_list_filename).readlines()
        worker_list_file = [line.split() for line in worker_list_file]
        for line in worker_list_file:
            if line[2] == '(nil)':
                self.worker_list.append(line[-1])

    def sanity_check_config(self, config_file):
        print("currently checking for most required features")
        config_lines = open(config_file).readlines()
        while (i < len(config_lines) - 1):
            if config_lines[i] == 'INSTALL':
                if config_lines[i + 1] != '0':
                    print("Install is true for config, can't run genstrat")
            elif config_lines[i] == 'MAIL_ADDRESS':
                if config_lines[i + 1]:
                    print("NonEmpty email address given. Please remove")

    def call_generate_strategies(self, config_file):
        self.sanity_check_config(config_file)
        exec_cmd = [self.generate_strategy, config_file]
        # currently not using functionalities of celery, treating it as any bash process
        celery_exec_cmd = self.distributed_executor + '-n dvctrader -m 1 -w 1 " '
        celery_exec_cmd = celery_exec_cmd.split() + exec_cmd + ['"']
        print(" ".join(celery_exec_cmd))
        out_file = subprocess.check_output(celery_exec_cmd)

        out_contents = open(out_file).readlines()
        out_contents = [line.split() for line in out_contents]
        for line in out_contents:
            if line[0] == 'Logfile':
                self.config_to_outdirectory[config_file] = line[1]

    def get_worker_for_logfile(self, logfile):
        for worker in self.worker_list:
            sshcmd = "ssh " + worker + " 'ls " + logfile + " ' "
            output = subprocess.check_output(sshcmd)
            if output:
                return worker

    def get_best_strategy_contents(self, worker, logfile):
        local_results_dir = os.path.dirname(logfile) + '/local_results_base_dir'
        cmd = self.summarize_local_results + "kCNAPnlSharpeAverage 10000 1000 -1 10 3000 -1 " + local_results_dir + " grep STRAT "
        ssh_cmd = "ssh " + worker + " ' " + cmd + " ' "
        output_lines = subprocess.check_output(ssh_cmd)
        strategy_lines = output_lines.split()
        if (len(strategy_lines) >= 0):
            print("Genstrat Could not generate strategy")
        else:
            strategy_name = strategy_lines[0].split()[1]

        strategy_name = subprocess.check_output(
            ['ssh', worker, "'", "ls", os.path.dirname("logfile") + "/*/" + "strategy_name", "'"])

        cmd = 'ssh ' + worker + "\"cat " + strategy_name + " | awk '{print $4}'\""
        model_name = subprocess.check_output(cmd.split())
        cmd = 'ssh ' + worker + "\"cat" + model_name + "\""
        model_content = subprocess.check_output(cmd.split())

        cmd = 'ssh ' + worker + "\"cat " + strategy_name + " | awk '{print $5}'\""
        param_name = subprocess.check_output(cmd.split())
        cmd = 'ssh ' + worker + "\"cat" + param_name + "\""
        param_content = subprocess.check_output(cmd.split())
        return (model_name, model_content, param_name, param_content)

    def check_output(self):
        for config_file in list(self.config_to_outdirectory.keys()):
            out_file_name = self.config_to_outdirectory[config_file]
            if os.path.exists(out_file_name) and os.path.getsize(out_file_name) > 0:
                logfile = open(out_file_name).readlines()[0]
                worker = self.get_worker_for_logfile(logfile)
                #model_name, model_content, param_name, param_content = self.get_best_strategy_contents(worker, logfile)
                config_to_strategy[config_file] = self.get_best_strategy_contents(worker, logfile)
                del self.config_to_outdirectory[config_file]

        return len(self.config_to_outdirectory) == 0
