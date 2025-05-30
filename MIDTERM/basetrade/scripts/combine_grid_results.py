#!/usr/bin/env python

import os
import shutil
import sys


def combine_artifacts_dir(results_directories, result_directory):
    if os.path.exists(result_directory):
        print("Deleting " + result_directory)
        shutil.rmtree(result_directory, ignore_errors=True)
    os.makedirs(result_directory)
    os.makedirs(os.path.join(result_directory, "artifacts"))
    os.makedirs(os.path.join(result_directory, "logs"))

    main_artifacts_dir = os.path.join(result_directory, "artifacts")
    main_logs_dir = os.path.join(result_directory, "logs")
    main_pnls_dir = os.path.join(main_artifacts_dir, "pnls")
    main_samples_dir = os.path.join(main_artifacts_dir, "samples")

    os.makedirs(main_pnls_dir)
    os.makedirs(main_samples_dir)

    for dir in results_directories:
        artifacts_dir = os.path.join(dir, "artifacts")
        logs_dir = os.path.join(dir, "logs")
        pnls_dir = os.path.join(artifacts_dir, "pnls")
        samples_dir = os.path.join(artifacts_dir, "samples")
        for pnl_result_file in os.listdir(pnls_dir):
            if os.path.isfile(os.path.join(pnls_dir, pnl_result_file)):
                pnl_result_file_handle = open(os.path.join(pnls_dir, pnl_result_file), 'r')
                pnls = pnl_result_file_handle.read()
                pnl_result_file_handle.close()

                combined_pnl_result_file_handle = open(os.path.join(main_pnls_dir, pnl_result_file), 'a')
                combined_pnl_result_file_handle.write(pnls)
                combined_pnl_result_file_handle.write("\n")
                combined_pnl_result_file_handle.close()

        for samples_result_file in os.listdir(samples_dir):
            if os.path.isfile(os.path.join(samples_dir, samples_result_file)):
                samples_result_file_handle = open(os.path.join(samples_dir, samples_result_file), 'r')
                samples = samples_result_file_handle.read()
                samples_result_file_handle.close()

                combined_samples_result_file_handle = open(os.path.join(main_samples_dir, samples_result_file), 'a')
                combined_samples_result_file_handle.write(samples)
                combined_samples_result_file_handle.write("\n")
                combined_samples_result_file_handle.close()

        for log_file in os.listdir(logs_dir):
            if os.path.isfile(os.path.join(logs_dir, log_file)):
                log_file_handle = open(os.path.join(logs_dir, log_file), 'r')
                logs = log_file_handle.read()
                log_file_handle.close()

                combined_logs_file_handle = open(os.path.join(main_logs_dir, log_file), 'a')
                combined_logs_file_handle.write(logs)
                combined_logs_file_handle.write("\n")
                combined_logs_file_handle.close()


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Either artifacts directory or result directory mentioned.")
        sys.exit(1)
    artifacts_directories = sys.argv[1:-1]
    result_directory = sys.argv[-1]
    combine_artifacts_dir(artifacts_directories, result_directory)
