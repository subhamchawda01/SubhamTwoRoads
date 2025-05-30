#!/usr/bin/env python

"""
maintains the list of execs
also we define all paths that can be referenced
"""

import os
import sys
import getpass
from socket import gethostname

from walkforward.utils.search_exec_or_script import search_script, search_exec


def get_temp_location():
    if gethostname().startswith('ip-10-0'):
        temp_location = "/media/shared/ephemeral16/" + getpass.getuser() + "/"
    elif 'WORKDIR' in os.environ:
        temp_location = os.path.join(os.environ.get('WORKDIR'), 'trash/')
    else:
        temp_location = "/home/" + getpass.getuser() + "/trash/"
    return temp_location


class paths(object):
    home = os.path.expanduser('~/')
    basetrade = os.path.join(home, 'basetrade')
    basetrade_install = os.path.join(home, 'basetrade_install')
    baseinfra = os.path.join(home, 'baseinfra')
    infracore = os.path.join(home, 'infracore')
    infracore_install = os.path.join(home, 'infracore_install')
    scripts = os.path.join(basetrade, 'scripts')
    model_scripts = os.path.join(basetrade, 'ModelScripts')
    install_bin = os.path.join(basetrade_install, 'bin')
    modelling = os.path.join(home, 'modelling')
    modelling_dvc = os.path.join("/home/dvctrader", 'modelling')
    walkforward = os.path.join(basetrade, 'walkforward')
    offlineconfigs = os.path.join(baseinfra, 'OfflineConfigs')

    tradeinfo = '/spare/local/tradeinfo/'

    p2y = os.path.join(tradeinfo, 'p2y')
    yield_data = os.path.join(p2y, 'yield_data')
    hctd = os.path.join(p2y, 'hctd')

    fbpa_user = '/spare/local/' + getpass.getuser() + '/'
    shared_ephemeral_fbpa = '/media/shared/ephemeral16/' + getpass.getuser() + '/FBPA/'
    shared_ephemeral_indicator_logs = '/media/shared/ephemeral16/' + getpass.getuser() + '/indicatorLogs/'
    celery_shared_cmds = '/media/shared/ephemeral16/' + getpass.getuser() + '/celery_cmds/'

    dpi_logs = '/media/shared/ephemeral1/s3_cache/NAS1/logs/DPILogs/'

    # to print temp model and temp params, when we extract strat out of config

    def __init__(self):
        self.home = os.path.expanduser('~/')
        self.basetrade = os.path.join(self.home, 'basetrade')
        self.baseinfra = os.path.join(self.home, 'baseinfra')
        self.basetrade_install = os.path.join(self.home, 'basetrade_install')
        self.infracore = os.path.join(self.home, 'infracore')
        self.infracore_install = os.path.join(self.home, 'infracore_install')
        self.scripts = os.path.join(self.basetrade, 'scripts')
        self.model_scripts = os.path.join(self.basetrade, 'ModelScripts')
        self.install_bin = os.path.join(self.basetrade_install, 'bin')
        self.modelling = os.path.join(self.home, 'modelling')
        self.walkforward = os.path.join(self.basetrade, 'walkforward')
        self.offlineconfigs = os.path.join(self.baseinfra, 'OfflineConfigs')


class execs(paths):

    def __init__(self):
        my_list_of_additional_paths = []

        self.summarize_strategy = search_exec('summarize_strategy_results')
        self.summarize_single_strategy = search_exec('summarize_single_strategy_results')
        self.summarize_local_results_dir_and_choose_by_algo = search_exec(
            'summarize_local_results_dir_and_choose_by_algo')

        self.ors_binary_reader = search_exec('ors_binary_reader')
        self.datagen = search_exec('datagen')
        self.get_min_price_increment = search_exec('get_min_price_increment')
        self.get_contract_specs = search_exec('get_contract_specs')
        self.get_exchange_symbol = search_exec('get_exchange_symbol')
        self.timed_data_to_reg_data = search_exec('timed_data_to_reg_data')
        self.timed_data_to_reg_data_bd = search_exec('timed_data_to_reg_data_bd')
        self.sim_strategy = search_exec('sim_strategy')
        self.daily_trade_aggregator = search_exec('daily_trade_aggregator')

        self.pred_counters = os.path.join(self.model_scripts, 'print_pred_counters_for_this_pred_algo.pl')
        self.apply_dep_filter = os.path.join(self.model_scripts, 'apply_dep_filter.pl')

        self.get_ind_stats_for_ilist = os.path.join(self.model_scripts, 'get_ind_stats_for_ilist.pl')

        self.print_strat_from_config = os.path.join(paths.walkforward, 'print_strat_from_config.py')
        self.ilist_to_correlation_to_DB = os.path.join(paths.model_scripts, 'ilist_to_correlation_to_DB.py')

        # regresion execs
        self.ridge = os.path.join(self.model_scripts, 'build_unconstrained_ridge_model.R')
        self.lm = os.path.join(self.model_scripts, 'build_linear_model.R')

        self.collect_shortcodes_from_model = os.path.join(self.install_bin, 'collect_shortcodes_from_model')
        self.holiday_manager = search_exec('holiday_manager')

        self.timed_data_to_corr_record = search_exec('timed_data_to_corr_record')
        self.get_price_info_for_day = search_exec('get_price_info_for_day')
        self.fsrr = search_exec('callFSRR')

        self.fsrlm = search_exec('callFSRLM')
        self.fsrmfss = search_exec('callfsr_mean_fss_corr')
        self.fsrmsh = search_exec('callfsr_mean_sharpe_corr')
        self.fsrshrsq = search_exec('callfsr_sharpe_rsq')

        self.lasso = os.path.join(self.model_scripts, 'call_lasso.pl')
        self.slasso = os.path.join(self.model_scripts, 'call_slasso.pl')

        self.nnls = os.path.join(self.model_scripts, 'build_nnls.R')

        self.siglr = os.path.join(self.model_scripts, 'SIGLR_grad_descent_3.R')
        self.fsrr = search_exec('callFSRR')

        self.get_stdev_model = os.path.join(self.model_scripts, 'get_stdev_model.pl')
        self.rescale_model = os.path.join(self.model_scripts, 'rescale_model.pl')

        self.get_yield = os.path.join(self.scripts, 'get_yield.R')

        # place coefss execs
        self.place_coeffs = os.path.join(self.model_scripts, 'place_coeffs_in_model.pl')
        self.place_coeffs_siglr = os.path.join(self.model_scripts, 'place_coeffs_in_siglr_model.pl')
        self.place_coeffs_lasso = os.path.join(self.model_scripts, 'place_coeffs_in_lasso_model.pl')
        self.place_coeffs_slasso = os.path.join(self.model_scripts, 'place_coeffs_in_slasso_model.pl')

        # date utils execs
        self.calc_prev_week_day = search_exec('calc_prev_week_day')
        self.calc_next_week_day = search_exec('calc_next_week_day')
        self.calc_next_day = search_exec('calc_next_day')
        self.calc_prev_day = search_exec('calc_prev_day')
        self.get_utc_hhmm = search_exec('get_utc_hhmm_str')

        self.mkt_trade_logger = search_exec('mkt_trade_logger')
        self.mkt_book_viewer = search_exec('mkt_book_viewer')
        self.get_path = search_exec('get_path')
        self.mds_log_reader = search_exec('mds_log_reader')

        # sample data and backtest-result generation scripts
        self.sample_data = os.path.join(self.scripts, 'get_avg_samples.pl')
        self.get_average_events_per_second = os.path.join(self.model_scripts, 'get_average_event_count_per_second.pl')
        self.run_simulations = os.path.join(self.model_scripts, 'run_simulations.pl')
        self.avg_samples = os.path.join(self.scripts, 'get_avg_samples.pl')

        self.run_strat = os.path.join(self.scripts, 'run_strat.sh')
        self.run_strat_and_plot = os.path.join(self.scripts, 'run_strat_and_plot.sh')
        self.run_accurate_sim_real = os.path.join(paths.scripts, 'run_accurate_sim_real.pl')
        self.show_recent_global_results = os.path.join(self.scripts, 'show_recent_global_results_2.sh')

        # di1 cbt  usage scripts
        self.di1_dv01_val = os.path.join(self.scripts, 'get_di1_straing_dv01.sh')

        # sample feature usage scripts
        self.dynamic_pred_dur = os.path.join(self.scripts, 'get_pred_dur_from_feature.py')

        # model market info and sim config
        self.market_model_info = os.path.join(paths.offlineconfigs, 'MarketModelInfo')
        self.sim_config = os.path.join(paths.offlineconfigs, 'SimConfig')

        # sim_config path used in check_strat_seq_delay_sensitivity
        self.sim_config_base = os.path.join("baseinfra", "OfflineConfigs", 'SimConfig')
        self.market_model_info_base = os.path.join("baseinfra", "OfflineConfigs", 'MarketModelInfo')

        # sample feature usage scripts
        self.dynamic_pred_dur = os.path.join(self.scripts, 'get_pred_dur_from_feature.py')

        # Economic events for product
        self.get_economic_events_for_product = os.path.join(self.scripts, 'get_economic_events_for_product.pl')

        # WKoDii features
        self.get_day_features = os.path.join(self.basetrade, 'WKoDii/get_day_features.pl')

        self.get_dates = os.path.join(self.scripts, 'get_dates_for_shortcode.pl')
        self.get_economic_events_for_product = os.path.join(self.scripts, 'get_economic_events_for_product.pl')

        self.get_day_features = os.path.join(self.basetrade, 'WKoDii/get_day_features.pl')
        self.run_simulations = os.path.join(self.model_scripts, 'run_simulations.pl')

        self.di1_dv01_val = os.path.join(self.scripts, 'get_di1_straing_dv01.sh')

        self.celery_run_job_exec = "/home/dvctrader/dvccode/scripts/datainfra/celeryFiles/celeryClient/celeryScripts/run_my_job.py"
        self.dynamic_pred_dur = os.path.join(self.scripts, 'get_pred_dur_from_feature.py')
        self.get_queue_position = os.path.join(self.scripts, 'get_queue_position.py')
        self.print_queue_stats = os.path.join(self.install_bin, 'print_queue_stats')
        self.get_cxl_rej_stats = os.path.join(self.scripts, 'get_cxl_rej_stats.py')
        self.get_cxl_rej_stats2 = os.path.join(self.scripts, 'get_cxl_rej_stats2.py')

        self.generate_multiple_indicator_stats_2_distributed = os.path.join(self.model_scripts, "generate_multiple_indicator_stats_2_distributed.py")
        self.pnl_based_stats_handler = os.path.join(self.scripts, "pnl_based_stats_handler.py")
        self.create_config_from_ifile = os.path.join(self.walkforward, "create_config_from_ifile.py")
