#!/usr/bin/env python
import os
import os.path
import sys
from subprocess import Popen, PIPE
import commands
import operator
from math import sqrt
from datetime import datetime

from getpass import getuser

from walkforward.utils.search_exec_or_script import search_script, search_exec

GENPYTHONLIB_DIR = "../PythonLib";  # Not being used TODO: remove later

HOME = "/home/" + getuser() + "/"
REPO = HOME + "basetrade/"
SPARE_LOCAL = "/spare/local/" + getuser() + "/study_of_learning_methods/"
cmd_ = "%s %s" % ("mkdir -p ", SPARE_LOCAL)
os.system(cmd_)

DATA_DIR = "/NAS1/data/study_of_learning_methods/archit/"
RSYNC_DIR = "dvcinfra@10.23.74.40:/apps/data/study_of_learning_methods/archit/"
BASETRADE_INSTALL = HOME + "basetrade_install/"
SIM_STRAT_TRADE_FILE_PRE = "/spare/local/logs/tradelogs/trades."
SIM_STRAT_LOG_FILE_PRE = "/spare/local/logs/tradelogs/log."
LIVE_BIN_DIR = BASETRADE_INSTALL + "bin/"
MODELSCRIPTS_DIR = BASETRADE_INSTALL + "ModelScripts/"
GENPERLLIB_DIR = BASETRADE_INSTALL + "GenPerlLib/"

if(os.path.isdir(HOME + 'LiveExec/bin/')):
    LIVE_BIN_DIR = HOME + 'LiveExec/bin/'

prog_id_ = int(commands.getoutput("date +%N"))
TMP_DIR = SPARE_LOCAL + str(prog_id_) + "/"
os.system("mkdir %s" % (TMP_DIR))
LOG_DIR = SPARE_LOCAL + "logs/"
os.system("mkdir -p %s" % (LOG_DIR))
log_file_ = LOG_DIR + "log_" + str(prog_id_) + ".txt"
LOG_FILEHANDLE = open(log_file_, 'w', 1)

COSTFUNCTION = {'Pnl': 1, 'PnlVolSqrt': 2, 'Vol': 3, 'TTC': 4, 'MaxDrawdown': 5}
#


class SimStatsSummary:
    def __init__(self):
        # position
        self.avg_abs_pos_ = 0

        # ttc
        self.median_ttc_ = 0
        self.avg_ttc_ = 0
        self.max_ttc_ = 0

        # vol
        self.vol_ = 0

        # pnl stats
        self.total_pnl_ = 0
        self.pnl_min_ = 0
        self.pnl_max_ = 0
        self.pnl_drawdown_ = 0
        self.median_pnl_ = 0
        self.avg_pnl_ = 0
        self.pnl_stdev_ = 0
        self.sharpe_pnl_ = 0
        self.fracpos_pnl_ = 0

        self.num_trades_ = 0
        self.count_ = 0

    def UpdateSum(self, sim_stats):
        # position
        self.avg_abs_pos_ += float(sim_stats[1])

        # ttc
        self.median_ttc_ += int(sim_stats[2])
        self.avg_ttc_ += int(sim_stats[3])
        self.max_ttc_ += int(sim_stats[12])

        # vol
        self.vol_ += int(sim_stats[14])

        # pnl stats
        self.total_pnl_ += int(sim_stats[13])
        self.pnl_min_ += int(sim_stats[9])
        self.pnl_max_ += int(sim_stats[10])
        self.pnl_drawdown_ = int(sim_stats[11])
        self.median_pnl_ += int(sim_stats[4])
        self.avg_pnl_ += int(sim_stats[5])
        self.pnl_stdev_ += int(sim_stats[6])
        self.sharpe_pnl_ += float(sim_stats[7])
        self.fracpos_pnl_ += float(sim_stats[8])

        self.num_trades_ += int(sim_stats[15])
        self.count_ += 1

    def Add(self, sim_stats_summary_):
        # position
        self.avg_abs_pos_ += sim_stats_summary_.avg_abs_pos_

        # ttc
        self.median_ttc_ += sim_stats_summary_.median_ttc_
        self.avg_ttc_ += sim_stats_summary_.avg_ttc_
        self.max_ttc_ += sim_stats_summary_.max_ttc_

        # vol
        self.vol_ += sim_stats_summary_.vol_

        # pnl stats
        self.total_pnl_ += sim_stats_summary_.total_pnl_
        self.pnl_min_ += sim_stats_summary_.pnl_min_
        self.pnl_max_ += sim_stats_summary_.pnl_max_
        self.pnl_drawdown_ += sim_stats_summary_.pnl_drawdown_
        self.median_pnl_ += sim_stats_summary_.median_pnl_
        self.avg_pnl_ += sim_stats_summary_.avg_pnl_
        self.pnl_stdev_ += sim_stats_summary_.pnl_stdev_
        self.sharpe_pnl_ += sim_stats_summary_.sharpe_pnl_
        self.fracpos_pnl_ += sim_stats_summary_.fracpos_pnl_

        self.num_trades_ += sim_stats_summary_.num_trades_
        self.count_ += 1

    def AverageOut(self):
        if((self.count_) == 0):
            return
        # position
        self.avg_abs_pos_ /= self.count_

        # ttc
        self.median_ttc_ /= self.count_
        self.avg_ttc_ /= self.count_
        self.max_ttc_ /= self.count_

        # vol
        self.vol_ /= self.count_

        # pnl stats
        self.total_pnl_ /= self.count_
        self.pnl_min_ /= self.count_
        self.pnl_max_ /= self.count_
        self.pnl_drawdown_ /= self.count_
        self.median_pnl_ /= self.count_
        self.avg_pnl_ /= self.count_
        self.pnl_stdev_ /= self.count_
        self.sharpe_pnl_ /= self.count_
        self.fracpos_pnl_ /= self.count_

        self.num_trades_ /= self.count_
        self.count_ = 1


def GetCost(sim_stats_summary_, cost_fn_):
    if(cost_fn_ == COSTFUNCTION['Pnl']):
        return sim_stats_summary_.total_pnl_
    elif(cost_fn_ == COSTFUNCTION['Vol']):
        return sim_stats_summary_.vol_
    elif(cost_fn_ == COSTFUNCTION['TTC']):
        return sim_stats_summary_.avg_ttc_
    elif(cost_fn_ == COSTFUNCTION['MaxDrawdown']):
        return sim_stats_summary_.pnl_drawdown_
    elif(cost_fn_ == COSTFUNCTION['PnlVolSqrt']):
        if(sim_stats_summary_.total_pnl_ >= 0 or sim_stats_summary_.vol_ == 0):
            return sim_stats_summary_.total_pnl_ * sqrt(sim_stats_summary_.vol_)
        if(sim_stats_summary_.total_pnl_ < 0):
            return sim_stats_summary_.total_pnl_ / (sqrt(sim_stats_summary_.vol_))
    else:
        return 0


def GetCostFromList(sim_stats_summary_list_, cost_fn_):
    data_size_ = len(sim_stats_summary_list_)
    if(data_size_ == 0):
        return 0
    cost_sum_ = 0
    if(cost_fn_ == COSTFUNCTION['Pnl'] or cost_fn_ == COSTFUNCTION['PnlVolSqrt'] or cost_fn_ == COSTFUNCTION['Vol'] or cost_fn_ == COSTFUNCTION['TTC'] or cost_fn_ == COSTFUNCTION['MaxDrawdown']):
        for sim_stats_summary_ in sim_stats_summary_list_:
            cost_sum_ += GetCost(sim_stats_summary_, cost_fn_)
        return cost_sum_ / data_size_
    else:
        return 0


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

    def GetIdentifierString(self):
        return "_".join(self.__str__().split())

#~ def MakeDataFile ( shortcode_, ilist_, date_list_, t_start_idx_, t_end_idx_, t_data_args_, t_output_file_, start_time_, end_time_ ):
    #~ #    os.system ("rm -f %s" % t_output_file_)
    #~ t_prefix_ = '_'.join(t_data_args_)
    #~ for idx_ in range ( t_start_idx_, t_end_idx_ + 1 ) :
        #~ t_source_file_ = DATA_DIR + shortcode_ +  "/" + t_prefix_ + str(date_list_[idx_])
        #~ #t_source_file_ = DATA_DIR + shortcode_ +  "/" + t_prefix_ + t_pred_duration_ + "_" + str(date_list_[idx_])
        #~ if not os.path.isfile ( t_source_file_ ) :
        #~ #t_source_file_local_ = SPARE_LOCAL + shortcode_ + "/" + t_prefix_ + t_pred_duration_ + "_" + str(date_list_[idx_])
        #~ t_source_file_local_ = SPARE_LOCAL + shortcode_ + "/" + t_prefix_ + str(date_list_[idx_])
        #~ GenRegDataFile (t_source_file_local_, shortcode_, ilist_, date_list_, idx_, t_prefix_, t_pred_duration_, t_filter_, start_time_, end_time_)
        #~ #os.system("rsync -avz %s %s" % (t_source_file_local_, RSYNC_DIR + shortcode_ + '/'))
        #~ os.system("rm %s" % (t_source_file_local_))
        #~ t_filtered_reg_data_ = TMP_DIR + "temp_filter_reg_data.txt"
#~
        #~ ApplyFilter(t_source_file_, t_filter_, t_filtered_reg_data_, date_list_[idx_] , shortcode_)
        #~ #if not os.path.isfile ( t_source_file_ ):    GenRegDataFile (t_source_file_, shortcode_, ilist_, date_list_, idx_, t_prefix_, start_time_, end_time_)
        #~ cat_cmd_ = "cat %s >> %s" % ( t_filtered_reg_data_, t_output_file_ )
        #~ #LOG_FILEHANDLE.write("Directory data location : %s\n" % (cat_cmd_))
        #~ os.system ( cat_cmd_ )
    #~ os.system("rm %s" % (t_filtered_reg_data_))


def MakeDataFile(shortcode_, ilist_, date_list_, t_start_idx_, t_end_idx_, t_data_args_, t_output_file_, start_time_, end_time_):
    for idx_ in range(t_start_idx_, t_end_idx_ + 1):
        t_filtered_reg_data_ = TMP_DIR + "temp_filter_reg_data.txt"
        GenRegDataFile(t_filtered_reg_data_, shortcode_, ilist_, date_list_, idx_, t_data_args_, start_time_, end_time_)
        cat_cmd_ = "cat %s >> %s" % (t_filtered_reg_data_, t_output_file_)
        os.system(cat_cmd_)
        os.system("rm %s" % (t_filtered_reg_data_))


def GenRegDataFile(filtered_reg_data_file_, shortcode_, ilist_, date_list_, idx_, t_data_args_, start_time_, end_time_):
    # generate timed_data
    timed_data_filename_ = "timed_data_" + \
        ilist_.split('/')[-1] + "_" + str(date_list_[idx_]) + "_" + start_time_ + \
        "_" + end_time_ + "_" + '_'.join(t_data_args_[2:6])
    timed_data_ = DATA_DIR + shortcode_ + "/" + timed_data_filename_
    if not os.path.isfile(timed_data_):
        timed_data_local_ = TMP_DIR + timed_data_filename_
        datagen_cmd_ = "%s %s %d %s %s %s %s %s %s %s %s" % (LIVE_BIN_DIR + "datagen ", ilist_, date_list_[
                                                             idx_], start_time_, end_time_, "138085080", timed_data_local_, t_data_args_[2], t_data_args_[3], t_data_args_[4], t_data_args_[5])
        LOG_FILEHANDLE.write("DATAGEN CMD : %s\n" % (datagen_cmd_))
        t_out_ = commands.getoutput(datagen_cmd_)
        LOG_FILEHANDLE.write("DATAGEN OUTPUT : %s\n" % (t_out_))
        os.system("rsync -avz %s %s" % (timed_data_local_, RSYNC_DIR + shortcode_ + '/'))
        os.system("rm %s" % (timed_data_local_))

    # generate reg_data
    pred_algo_ = t_data_args_[0]
    pred_duration_ = t_data_args_[1]
    #pred_counters_ = int(float(commands.getoutput("perl %s %s %s %s %s" % ("get_pred_counters_for_this_pred_algo.pl", shortcode_, pred_duration_, pred_algo_, timed_data_ ))))
    print_pred_counters_for_this_pred_algo_script = search_script('print_pred_counters_for_this_pred_algo.pl')
    pred_counters_ = int(float(commands.getoutput("%s %s %s %s %s" % (
        print_pred_counters_for_this_pred_algo_script, shortcode_, pred_duration_, pred_algo_, timed_data_, start_time_, end_time_))))
    t_reg_data_ = TMP_DIR + "temp_reg_data.txt"
    reg_data_cmd_ = "%s %s %s %s %s %s" % (LIVE_BIN_DIR + "timed_data_to_reg_data",
                                           ilist_, timed_data_, pred_counters_, pred_algo_, t_reg_data_)
    LOG_FILEHANDLE.write("REGDATA CMD : %s\n" % (reg_data_cmd_))
    t_out_ = commands.getoutput(reg_data_cmd_)
    LOG_FILEHANDLE.write("REGDATA OUTPUT : %s\n" % (t_out_))

    # applying filter
    t_filter_ = t_data_args_[-1]
    exec_cmd = ("%s %s %s %s %s %s" % (MODELSCRIPTS_DIR + "apply_dep_filter.pl", shortcode_,
                                       t_reg_data_, t_filter_, filtered_reg_data_file_, date_list_[idx_]))
    t_out_ = commands.getoutput(exec_cmd)
    os.system("rm %s" % (t_reg_data_))


def MakeStrategyFile(t_shortcode_, t_exec_logic, t_mfile_, t_param_file_, t_start_time_, t_end_time_, t_output_filename_):
    STRATFILE = open(t_output_filename_, 'w')
    STRATFILE.write("STRATEGYLINE %s %s %s %s %s %s 30011" %
                    (t_shortcode_, t_exec_logic, t_mfile_, t_param_file_, t_start_time_, t_end_time_))
    STRATFILE.close()

# IsValidDate not being called


def IsValidDate(trading_date_, ilist_filename_):
    is_valid_ = int(commands.getoutput("perl %s %s %s" %
                                       (GENPERLLIB_DIR + "is_valid.pl", trading_date_, ilist_filename_)).strip())
    if(is_valid_ == 1):
        return True
    else:
        return False


def GetDateList(start_date_, end_date_, ilist_filename_):
    date_list_ = []
    for date_ in range(start_date_, end_date_ + 1):
        #is_valid_ = int(commands.getoutput("perl %s %s %s" % ("is_valid.pl", date_, ilist_filename_ )).strip())
        is_valid_ = int(commands.getoutput("perl %s %s %s" %
                                           (GENPERLLIB_DIR + "is_valid.pl", date_, ilist_filename_)).strip())
        if(is_valid_ == 1):
            date_list_.append(date_)
    return date_list_


def GetStratString(t_algo_, t_param_file_, t_exec_logic_):
    return t_algo_.GetIdentifierString() + "_" + t_param_file_.split('/')[-1] + "_" + t_exec_logic_

#


def __main__():
    if len(sys.argv) < 20:
        print "USAGE: $0 shortcode ilist start_date_ end_datie_ learning_set_length test_set_length algo_names_filename_ output_filename_ param_execlogic_list_file trading_start_time_ trading_end_time_ cost_fn_(", COSTFUNCTION.keys(), ") filter_ pred_dur_ pred_algo_ MSECS_PRINT l1EVENTS_PRINT NUM_TRADES_PRINT ECO_MODE"
        exit()

    #
    shortcode_ = sys.argv[1]
    ilist_filename_ = sys.argv[2]
    remote_ilist_ = DATA_DIR + shortcode_ + "/" + ilist_filename_.split('/')[-1]
    if(os.path.isfile(remote_ilist_)):
        diff = commands.getoutput("diff %s %s" % (ilist_filename_, remote_ilist_))
        if(diff):
            print "data for ilist with same name already exists but ilist file has changed, specify a new name for this ilist"
            exit()
    else:
        os.system("rsync -avz %s %s" % (ilist_filename_, RSYNC_DIR + shortcode_ + '/'))

    start_date_ = int(sys.argv[3])
    end_date_ = int(sys.argv[4])
    learning_set_length_ = int(sys.argv[5])
    test_set_length_ = int(sys.argv[6])
    algo_names_filename_ = sys.argv[7]
    output_filename_ = sys.argv[8]
    param_file_ = sys.argv[9]
    start_time_ = sys.argv[10]
    end_time_ = sys.argv[11]
    cost_fn_ = sys.argv[12]
    if(cost_fn_ not in COSTFUNCTION):
        print "invalid cost function"
        exit()
    filter_ = sys.argv[13]
    pred_dur_ = sys.argv[14]
    pred_algo_ = sys.argv[15]
    datagen_timeouts_ = [sys.argv[16], sys.argv[17], sys.argv[18], sys.argv[19]]

    data_args_ = [pred_algo_, pred_dur_, datagen_timeouts_[0], datagen_timeouts_[
        1], datagen_timeouts_[2], datagen_timeouts_[3], filter_]
    data_prefix_ = '_'.join(data_args_)

    RESULTS = open(output_filename_, 'w', 1)
    RESULTS.write("PROG_ID : %d\nTMP_DIR : %s\nlog_file : %s\n" % (prog_id_, TMP_DIR, log_file_))

    LOG_FILEHANDLE.write("generating date_list\n")
    date_list_ = GetDateList(start_date_, end_date_, ilist_filename_)
    RESULTS.write("start_date : %d, end_date : %d, total_days : %d\n" %
                  (date_list_[0], date_list_[-1], len(date_list_)))

    hist_start_idx_ = 0
    hist_end_idx_ = learning_set_length_ - 1
    test_start_idx_ = learning_set_length_
    test_end_idx_ = learning_set_length_ + test_set_length_ - 1

    learning_algo_list_ = []
    algo_cat_str_to_cost_ = {}    # for storing the final cost function
    sim_stats_summary_per_strat_per_date_ = []
    param_exec_logic_pair_list_ = []

    LOG_FILEHANDLE.write("loading algo_file\n")
    ALGO_NAMES_FILEHANDLE = open(algo_names_filename_, 'r')
    lines = [line.strip() for line in ALGO_NAMES_FILEHANDLE.readlines()]
    for line in lines:
        algo_string_ = line.split()
        arg_string_ = ' '.join(algo_string_[3:-2]);
        new_algo_ = LearningMethod(algo_string_[0], algo_string_[1], data_prefix_,
                                   arg_string_, int(algo_string_[-2]), int(algo_string_[-1]))
        learning_algo_list_.append(new_algo_)
    ALGO_NAMES_FILEHANDLE.close()

    LOG_FILEHANDLE.write("loading param_file\n")
    PARAM_FILEHANDLE = open(param_file_, 'r')
    lines = [line.strip() for line in PARAM_FILEHANDLE.readlines()]
    for line in lines:
        t_param_exec_logic_pair_ = line.split()
        if (os.path.isfile(t_param_exec_logic_pair_[0])):
            param_exec_logic_pair_list_.append(t_param_exec_logic_pair_)
        else:
            LOG_FILEHANDLE.write("param_file %s not found\n" % (t_param_exec_logic_pair_[0]))
    PARAM_FILEHANDLE.close()

    strat_str_to_strat_id_ = {}
    strat_id_to_strat_str_ = []
    t_strat_id = 0
    for t_algo_idx_ in range(len(learning_algo_list_)):
        for t_param_exec_logic_pair_idx_ in range(len(param_exec_logic_pair_list_)):
            strat_str_to_strat_id_[str(t_algo_idx_) + "_" + str(t_param_exec_logic_pair_idx_)] = t_strat_id
            strat_id_to_strat_str_.append(GetStratString(learning_algo_list_[t_algo_idx_], param_exec_logic_pair_list_[
                                          t_param_exec_logic_pair_idx_][0], param_exec_logic_pair_list_[t_param_exec_logic_pair_idx_][1]))
            sim_stats_summary_per_strat_per_date_.append([])
            t_strat_id += 1

    files_to_remove_ = []

    RESULTS.write("ALGORITHM_IDENTIFIER_STRING\t\t\t\t\t\t COST(%s)\t Pnl\t Vol\t TTC\t MaxDrawdown\n" % (cost_fn_))
    while test_end_idx_ < len(date_list_):

        strat_filename_ = TMP_DIR + shortcode_ + "_strat_file_" + \
            str(date_list_[test_start_idx_]) + "_" + str(date_list_[test_end_idx_]) + ".txt"
        #files_to_remove_.append ( strat_filename_ )
        STRAT_FILEHANDLE = open(strat_filename_, 'w', 1)
        t_sim_stats_summary_list_ = []
        training_datafiles_ = []

        LOG_FILEHANDLE.write("###################Training Period : %d-%d, Testing Period : %d-%d####################\n" %
                             (date_list_[hist_start_idx_], date_list_[hist_end_idx_], date_list_[test_start_idx_], date_list_[test_end_idx_]))
        RESULTS.write("#####################Training Period : %d-%d, Testing Period : %d-%d#################\n" %
                      (date_list_[hist_start_idx_], date_list_[hist_end_idx_], date_list_[test_start_idx_], date_list_[test_end_idx_]))
        # creating model for each algo and appending a strategy for each model
        for t_algo_idx_ in range(len(learning_algo_list_)):
            t_algo_ = learning_algo_list_[t_algo_idx_]

            training_data_file_ = TMP_DIR + shortcode_ + "_training_data_" + data_prefix_ + "_" + \
                str(date_list_[hist_start_idx_]) + "_" + str(date_list_[hist_end_idx_]) + ".txt"
            if not os.path.isfile(training_data_file_):
                MakeDataFile(shortcode_, ilist_filename_, date_list_, hist_start_idx_,
                             hist_end_idx_, data_args_, training_data_file_, start_time_, end_time_)
                training_datafiles_.append(training_data_file_)

            if os.path.isfile(training_data_file_):
                # Run regress
                output_model_ = TMP_DIR + shortcode_ + "_reg_res_" + \
                    str(t_algo_idx_) + "_" + str(date_list_[hist_start_idx_]) + \
                    "_" + str(date_list_[hist_end_idx_]) + ".txt"
                files_to_remove_.append(output_model_)
                if t_algo_.name_ == "LASSO":
                    if t_algo_.match_corrs_ == 1:
                        regress_cmd_ = "%s %s %d %s %s" % (
                            t_algo_.regress_exec_, training_data_file_, t_algo_.max_model_size_, output_model_, ilist_filename_)
                    else:
                        regress_cmd_ = "%s %s %d %s" % (
                            t_algo_.regress_exec_, training_data_file_, t_algo_.max_model_size_, output_model_)
                elif t_algo_.name_ == "SIGLR":
                    if t_algo_.match_corrs_ == 1:
                        regress_cmd_ = "%s %s %s %d %s INVALIDFILENAME %s" % (
                            t_algo_.regress_exec_, training_data_file_, output_model_, t_algo_.max_model_size_, t_algo_.args_str_, ilist_filename_)
                    else:
                        regress_cmd_ = "%s %s %s %d %s INVALIDFILENAME " % (
                            t_algo_.regress_exec_, training_data_file_, output_model_, t_algo_.max_model_size_, t_algo_.args_str_)
                else:
                    if t_algo_.match_corrs_ == 1:
                        regress_cmd_ = "%s %s %s %s %d INVALIDFILENAME %s" % (
                            t_algo_.regress_exec_, training_data_file_, t_algo_.args_str_, output_model_, t_algo_.max_model_size_, ilist_filename_)
                    else:
                        regress_cmd_ = "%s %s %s %s %d INVALIDFILENAME " % (
                            t_algo_.regress_exec_, training_data_file_, t_algo_.args_str_, output_model_, t_algo_.max_model_size_)

            LOG_FILEHANDLE.write("Regress strat time %s \n" % str(datetime.now()))
            LOG_FILEHANDLE.write("REGRESSION COMMAND %d: %s \n" % (t_algo_idx_, regress_cmd_))
            t_out_ = commands.getoutput(regress_cmd_)
            LOG_FILEHANDLE.write("REGRESSION OUTPUT %d: %s \n" % (t_algo_idx_, t_out_))
            LOG_FILEHANDLE.write("Regress end time %s \n" % str(datetime.now()))

            unscaled_mfile_ = TMP_DIR + shortcode_ + "_unscaled_m_file_" + \
                str(t_algo_idx_) + "_" + str(date_list_[hist_start_idx_]) + \
                "_" + str(date_list_[hist_end_idx_]) + ".txt"
            #files_to_remove_.append ( mfile_ )
            if(t_algo_.name_ == "LASSO"):
                place_coeffs_script_ = "place_coeffs_in_lasso_model.pl"
            elif(t_algo_.name_ == "MLOGIT"):
                place_coeffs_script_ = "place_coeffs_in_logit_model.pl"
            elif(t_algo_.name_ == "EARTH"):
                place_coeffs_script_ = "place_coeffs_in_mars_models.py"
            elif(t_algo_.name_ == "SIGLR"):
                place_coeffs_script_ = "place_coeffs_in_siglr_model.pl"
            else:
                place_coeffs_script_ = "place_coeffs_in_model.pl"
            m_file_cmd_ = "%s %s %s %s" % (MODELSCRIPTS_DIR + place_coeffs_script_,
                                           unscaled_mfile_, ilist_filename_, output_model_)
            LOG_FILEHANDLE.write("PLACE_COEFF COMMAND %d: %s \n" % (t_algo_idx_, m_file_cmd_))
            t_out_ = commands.getoutput(m_file_cmd_)
            LOG_FILEHANDLE.write("PLACE_COEFF OUTPUT %d: %s \n" % (t_algo_idx_, t_out_))

            if os.path.isfile(unscaled_mfile_):
                for t_param_exec_logic_pair_idx_ in range(len(param_exec_logic_pair_list_)):
                    param_exec_logic_pair_ = param_exec_logic_pair_list_[t_param_exec_logic_pair_idx_]
                    scaled_mfile_ = TMP_DIR + shortcode_ + "_scaled_m_file_" + str(t_algo_idx_) + "_" + str(
                        t_param_exec_logic_pair_idx_) + "_" + str(date_list_[hist_start_idx_]) + "_" + str(date_list_[hist_end_idx_]) + ".txt"
                    l1norm_model_cmd_ = "perl %s %s %s %s %s %s | head -n1" % (MODELSCRIPTS_DIR + "get_stdev_model.pl", unscaled_mfile_, str(
                        date_list_[hist_start_idx_]), str(date_list_[hist_end_idx_]), start_time_, end_time_)
                    model_norm_ = commands.getoutput(l1norm_model_cmd_)
                    current_stdev_ = float(model_norm_.split(' ')[0])
                    current_l1norm_ = float(model_norm_.split(' ')[1])
                    desired_l1norm_ = float(param_exec_logic_pair_[2])
                    scale_const_ = desired_l1norm_ / current_l1norm_
                    scale_model_cmd_ = "perl %s %s %s %s " % (
                        MODELSCRIPTS_DIR + "rescale_model.pl", unscaled_mfile_, scaled_mfile_, scale_const_)
                    LOG_FILEHANDLE.write("RESCALE MODEL COMMAND %d %d: %s \n" %
                                         (t_algo_idx_, t_param_exec_logic_pair_idx_, scale_model_cmd_))
                    t_out_ = commands.getoutput(scale_model_cmd_)
                    LOG_FILEHANDLE.write("RESCALE OUTPUT %d %d: %s \n" %
                                         (t_algo_idx_, t_param_exec_logic_pair_idx_, t_out_))
                    STRAT_FILEHANDLE.write("STRATEGYLINE %s %s %s %s %s %s %d\n" % (shortcode_, param_exec_logic_pair_[1], scaled_mfile_, param_exec_logic_pair_[
                                           0], start_time_, end_time_, strat_str_to_strat_id_[str(t_algo_idx_) + "_" + str(t_param_exec_logic_pair_idx_)]))
            else:
                LOG_FILEHANDLE.write("UNSCALED MFILE %s not found\n" % (unscaled_mfile_))

        STRAT_FILEHANDLE.close()

        t_sim_stats_summary_per_strat_per_date_ = []
        for t_strat_id_ in range(len(strat_str_to_strat_id_)):
            t_sim_stats_summary_per_strat_per_date_.append([])

        # added preemptive removal of training data files
        for fname_ in training_datafiles_:
            if os.path.isfile(fname_):
                os.remove(fname_)

        for trading_date_idx_ in range(test_start_idx_, test_end_idx_ + 1):

            trading_date_ = date_list_[trading_date_idx_]
            sim_strat_cmd_ = "%s SIM %s %s %s" % (LIVE_BIN_DIR + "sim_strategy",
                                                  strat_filename_, prog_id_, trading_date_)
            LOG_FILEHANDLE.write("SIM_STRAT COMMAND for %d: %s \n" % (trading_date_, sim_strat_cmd_))
            t_out_ = commands.getoutput(sim_strat_cmd_)
            LOG_FILEHANDLE.write("SIM_STRAT OUTPUT for %d: %s \n" % (trading_date_, t_out_))

            sim_strat_trade_filename_ = SIM_STRAT_TRADE_FILE_PRE + str(trading_date_) + "." + str(prog_id_)
            sim_strat_log_filename_ = SIM_STRAT_LOG_FILE_PRE + str(trading_date_) + "." + str(prog_id_)
            exec_cmd_ = "%s %s %s" % (MODELSCRIPTS_DIR + "get_pnl_stats_multiple_strategy.pl",
                                      sim_strat_trade_filename_, 'E')
            #exec_cmd_ = "%s %s %s" % ( "perl get_pnl_stats_multiple_strategy.pl", sim_strat_trade_filename_, 'E' )
            os.system("rm -f %s" % (sim_strat_trade_filename_))
            os.system("rm -f %s" % (sim_strat_log_filename_))
            LOG_FILEHANDLE.write("GET_PNL_STATS COMMAND : %s \n" % (exec_cmd_))
            sim_stats_all_ = commands.getoutput(exec_cmd_).split('\n')
            LOG_FILEHANDLE.write("GET_PNL_STATS RESULT : %s \n" % (sim_stats_all_))
            for sim_stats_ in sim_stats_all_:
                sim_stats_ = sim_stats_.strip().split(' ')
                if(len(sim_stats_) < 16):
                    continue
                strat_id_ = int(sim_stats_[0])
                t_sim_stats_summary_per_strat_per_date_[strat_id_].append(SimStatsSummary())
                t_sim_stats_summary_per_strat_per_date_[strat_id_][-1].UpdateSum(sim_stats_)

        LOG_FILEHANDLE.write("SUMMARY for SIM_STRATS from %d-%d\n" %
                             (date_list_[test_start_idx_], date_list_[test_end_idx_]))
        for t_strat_id_ in range(len(strat_str_to_strat_id_)):
            t_data_ = t_sim_stats_summary_per_strat_per_date_[t_strat_id_]
            t_cost_ = [GetCostFromList(t_data_, COSTFUNCTION[cost_fn_]), GetCostFromList(t_data_, COSTFUNCTION['Pnl']), GetCostFromList(
                t_data_, COSTFUNCTION['Vol']), GetCostFromList(t_data_, COSTFUNCTION['TTC']), GetCostFromList(t_data_, COSTFUNCTION['MaxDrawdown'])]
            sim_stats_string_ = "SIM_STRAT_SUMMARY : %s\t %f\t %d\t %d\t %d\t %d\t \n" % (
                strat_id_to_strat_str_[t_strat_id_], t_cost_[0], t_cost_[1], t_cost_[2], t_cost_[3], t_cost_[4])
            RESULTS.write(sim_stats_string_)
            LOG_FILEHANDLE.write(sim_stats_string_)
            sim_stats_summary_per_strat_per_date_[
                t_strat_id_] = sim_stats_summary_per_strat_per_date_[t_strat_id_] + t_data_

        RESULTS.write("######################END OF A TESTING ITERATION#########################\n")

        hist_start_idx_ += test_set_length_
        hist_end_idx_ += test_set_length_
        test_start_idx_ += test_set_length_
        test_end_idx_ += test_set_length_
        # added preemptive removal of files
        for fname_ in files_to_remove_:
            if os.path.isfile(fname_):
                os.remove(fname_)

    LOG_FILEHANDLE.close()

    for fname_ in files_to_remove_:
        if os.path.isfile(fname_):
            os.remove(fname_)

    for t_strat_id_ in range(len(strat_str_to_strat_id_)):
        strat_str_ = strat_id_to_strat_str_[t_strat_id_]
        t_data_ = sim_stats_summary_per_strat_per_date_[t_strat_id_]
        t_cost_ = [GetCostFromList(t_data_, COSTFUNCTION[cost_fn_]), GetCostFromList(t_data_, COSTFUNCTION['Pnl']), GetCostFromList(
            t_data_, COSTFUNCTION['Vol']), GetCostFromList(t_data_, COSTFUNCTION['TTC']), GetCostFromList(t_data_, COSTFUNCTION['MaxDrawdown'])]
        sim_stats_string_ = "SUMMARY : %s\t %f\t %d\t %d\t %d\t %d\t \n" % (
            strat_str_, t_cost_[0], t_cost_[1], t_cost_[2], t_cost_[3], t_cost_[4])
        RESULTS.write(sim_stats_string_)
        algo_cat_str_to_cost_[strat_str_] = t_cost_

    final_sorted_list_ = sorted(algo_cat_str_to_cost_.iteritems(), key=operator.itemgetter(1))
    RESULTS.write("################################SORTED BY COST################################\n")
    for algo_cost_pair in final_sorted_list_:
        algo_identifier_string_ = algo_cost_pair[0]
        RESULTS.write("SUMMARY: %s\t %f\t %d\t %d\t %d\t %d\n" % (algo_identifier_string_,
                                                                  algo_cost_pair[1][0], algo_cost_pair[1][1], algo_cost_pair[1][2], algo_cost_pair[1][3], algo_cost_pair[1][4]));

    RESULTS.close()
    # os.system("rmdir %s" % (TMP_DIR))


#
__main__()
