#!/usr/bin/env python
import os
import os.path
import sys
from subprocess import Popen, PIPE

GENPYTHONLIB_DIR = "../PythonLib"


class LearningMethod:
    def __init__(self, t_name_, t_regress_exec_, t_data_prefix_, t_args_str_, t_max_model_size_, t_match_corrs_):
        self.name_ = t_name_
        self.regress_exec_ = t_regress_exec_
        self.data_prefix_ = t_data_prefix_
        self.args_str_ = t_args_str_
        self.max_model_size_ = t_max_model_size_
        self.match_corrs_ = t_match_corrs_

    def __str__(self):
        retstr_ = "D(" + self.data_prefix_ + ")" + self.name_ + "_" + self.args_str_ + "_M_" + str(self.max_model_size_)
        if self.match_corrs_ == 1:
            retstr_ += "_H"
        else:
            retstr_ += "_N"
        return retstr_


def MakeDataFile(date_list_, t_start_idx_, t_end_idx_, t_prefix_, t_output_file_):
    #    os.system ("rm -f %s" % t_output_file_)
    for idx_ in range(t_start_idx_, t_end_idx_ + 1):
        t_source_file_ = t_prefix_ + str(date_list_[idx_])
        cat_cmd_ = "cat %s >> %s" % (t_source_file_, t_output_file_)
        # print cat_cmd_,"\n"
        os.system(cat_cmd_)

#


def __main__():
    if len(sys.argv) < 6:
        print "USAGE: $0 ilist date_list_filename learning_set_length test_set_length algo_names_filename_"
        exit()

    #
    ilist_filename_ = sys.argv[1]
    date_list_filename_ = sys.argv[2]
    learning_set_length_ = int(sys.argv[3])
    test_set_length_ = int(sys.argv[4])
    algo_names_filename_ = sys.argv[5]
    DATE_LIST_FILEHANDLE = open(date_list_filename_, 'r')
    #
    date_list_ = DATE_LIST_FILEHANDLE.read().strip().split()
    date_list_ = [int(e) for e in date_list_]

    hist_start_idx_ = 0
    hist_end_idx_ = learning_set_length_ - 1
    test_start_idx_ = learning_set_length_
    test_end_idx_ = learning_set_length_ + test_set_length_ - 1

    learning_algo_list_ = []
    ALGO_NAMES_FILEHANDLE = open(algo_names_filename_, 'r')
    lines = [line.strip() for line in ALGO_NAMES_FILEHANDLE.readlines()]
    for line in lines:
        algo_string_ = line.split()
        arg_string_ = ' '.join(algo_string_[3:-2]);
        new_algo_ = LearningMethod(algo_string_[0], algo_string_[1], algo_string_[
                                   2], arg_string_, int(algo_string_[-2]), int(algo_string_[-1]))
        learning_algo_list_.append(new_algo_)

    files_to_remove_ = []

    while test_end_idx_ < len(date_list_):
        for t_algo_ in learning_algo_list_:
            data_prefix_ = t_algo_.data_prefix_

            training_data_file_ = str(date_list_[hist_start_idx_]) + "_" + \
                str(date_list_[hist_end_idx_]) + "_" + data_prefix_ + ".txt"
            files_to_remove_.append(training_data_file_)
            if not os.path.isfile(training_data_file_):
                MakeDataFile(date_list_, hist_start_idx_, hist_end_idx_, data_prefix_, training_data_file_)

            if os.path.isfile(training_data_file_):
                # Run regress
                output_model_ = "reg_res_" + t_algo_.name_ + "_" + \
                    str(date_list_[hist_start_idx_]) + "_" + \
                    str(date_list_[hist_end_idx_]) + "_" + data_prefix_ + ".txt"
                if t_algo_.match_corrs_ == 1:
                    regress_cmd_ = "%s %s %s %s %d INVALIDFILENAME %s" % (
                        t_algo_.regress_exec_, training_data_file_, t_algo_.args_str_, output_model_, t_algo_.max_model_size_, ilist_filename_)
                else:
                    regress_cmd_ = "%s %s %s %s %d INVALIDFILENAME " % (
                        t_algo_.regress_exec_, training_data_file_, t_algo_.args_str_, output_model_, t_algo_.max_model_size_)
                os.system(regress_cmd_)

                # predict data
                test_data_file_ = str(date_list_[test_start_idx_]) + "_" + \
                    str(date_list_[test_end_idx_]) + "_" + data_prefix_ + ".txt"
                if not os.path.isfile(test_data_file_):
                    MakeDataFile(date_list_, test_start_idx_, test_end_idx_, data_prefix_, test_data_file_)

                if os.path.isfile(test_data_file_):
                    files_to_remove_.append(test_data_file_)
                    yhat_file_ = "yhat_" + t_algo_.name_ + "_" + \
                        str(date_list_[test_start_idx_]) + "_" + \
                        str(date_list_[test_end_idx_]) + "_" + data_prefix_ + ".txt"
                    pred_reg_cmd_ = "/home/gchak/basetrade/ModelScripts/predict_reg_data.py %s %s %s" % (
                        test_data_file_, output_model_, yhat_file_)
                    # print pred_reg_cmd_,"\n"
                    os.system(pred_reg_cmd_)
                    if os.path.isfile(yhat_file_):
                        # calculate mse on test data
                        # print "/home/gchak/basetrade/ModelScripts/get_mse.py",yhat_file_,"\n"
                        mse_output_ = Popen(["/home/gchak/basetrade/ModelScripts/get_mse.py",
                                             yhat_file_], stdout=PIPE).communicate()[0]
                        mse_output_ = mse_output_.strip()
                        print("MSE: %s %s %s" % (mse_output_, date_list_[test_start_idx_], str(t_algo_)))

        hist_start_idx_ += test_set_length_
        hist_end_idx_ += test_set_length_
        test_start_idx_ += test_set_length_
        test_end_idx_ += test_set_length_

    for fname_ in files_to_remove_:
        os.remove(fname_)


#
__main__()
