import argparse
import sys
import os
import random
import subprocess
import pandas as pd
import io
from utils import silent_remove
import time

def get_result(summarize_strategy_exec_, product_, strat_list_file_, results_dir_,
               start_date_, end_date_, skip_dates_file_, sort_algo_, specific_dates_file_, _is_summary_):
    output_ = ""
    try:
        product_.replace('&', '\&')
        product ='"{}"'.format(product_)
        output_ = subprocess.getoutput(" ".join([summarize_strategy_exec_, product, strat_list_file_, results_dir_,
                                                start_date_, end_date_, skip_dates_file_, sort_algo_, "0",
                                                 specific_dates_file_, _is_summary_]))
    except subprocess.CalledProcessError as e:
        print(e.returncode)
    except Exception as e:
        print(e)
    return output_


def get_results_for_single_prod(summarize_strategy_exec_, product_, strats_list, strat_list_file_,
                                results_dir_, start_date_, end_date_, skip_dates_file_,
                                sort_algo_, specific_dates_file_):
    output_ = ""
    try:
        strats_list_ = [y + "_" + product_ for y in strats_list]
        with open(strat_list_file_, "w") as outpt:
            outpt.write('\n'.join(strats_list_))

        product_ = product_.replace('&', '\&')
        product = '"{}"'.format(product_)
        output_ = subprocess.getoutput(" ".join([summarize_strategy_exec_, product, strat_list_file_, results_dir_,
                                                 start_date_, end_date_, skip_dates_file_, sort_algo_, "0",
                                                 specific_dates_file_, "1"]))

    except subprocess.CalledProcessError as e:
        print(e.returncode)
    except Exception as e:
        print(e)
    if output_ == "":
        return pd.DataFrame([], columns=list('abcdefghijklmnopqrstuvwxyzABCDE'))
    else:
        df_ = pd.read_csv(io.StringIO(output_), delim_whitespace=True, header=None, error_bad_lines=False,
                          names=list('abcdefghijklmnopqrstuvwxyzABCDE'))
        df_['a'] = df_['b'].apply(lambda z: z.replace("_" + product_, ""))
        df_['b'] = df_['b'].apply(lambda z: product_)
    return df_


def get_prod_wise_results_for_strats(summarize_strategy_exec_, product_list_, strats_list, strat_list_file_,
                                     results_dir_, start_date_, end_date_, skip_dates_file_,
                                     sort_algo_, specific_dates_file_):
    list_of_results_df_ = [get_results_for_single_prod(summarize_strategy_exec_, prod, strats_list, strat_list_file_,
                                                       results_dir_, start_date_, end_date_, skip_dates_file_,
                                                       sort_algo_, specific_dates_file_) for prod in product_list_]

    df_total_ = pd.concat(list_of_results_df_)
    return df_total_


def get_result_string_for_all_prod_(row, out_df_, error_content_, hit_content_, sl_content_, start_date_, end_date_):
    s = ' '.join(row.to_csv(header=False, index=False, sep=' ').split('\n'))
    s += "\n"
    strat_df_ = out_df_[out_df_['a'] == row['a']]
    sorted_df_ = strat_df_.sort_values(by=['f'], ascending=False)
    s += sorted_df_.to_csv(header=False, index=False, sep=' ')

    error_list_ = [1 for line_ in error_content_ if line_[0] == row['a']
                   and start_date_ <= line_[1] <= end_date_]
    hit_list_ = [1 for line_ in hit_content_ if line_[0] == row['a']
                 and start_date_ <= line_[1] <= end_date_]
    sl_list_ = [1 for line_ in sl_content_ if line_[0] == row['a']
                and start_date_ <= line_[1] <= end_date_]

    error_count_ = len(error_list_)
    hit_count_ = len(hit_list_)
    sl_count_ = len(sl_list_)

    s += "Error Count: " + str(error_count_) + " Hit Count: " + str(hit_count_) +\
         " SL Count: " + str(sl_count_) + "\n"
    return s


skip_dates_file = "IF"
sort_algo = "kCNAPnlSharpeAverage"
specific_dates_file = "IF"
summarize_strategy_exec = "/home/dvctrader/stable_exec/summarize_strategy_results_detail"
product_list_file = ""

parser = argparse.ArgumentParser()
parser.add_argument('product', help='Product for which to summarize result')
parser.add_argument('strat_folder_or_file', help='Folder containing strats or file with strat names')
parser.add_argument('results_dir', help='Directory where results are stored')
parser.add_argument('start_date', help='Date from which results are to be considered')
parser.add_argument('end_date', help='Date till which results are to be considered')
parser.add_argument('--skip_dates_file', help='Dates to be skipped from consideration')
parser.add_argument('--sort_algo', help='Algo on which results will be sorted')
parser.add_argument('--specific_dates_file', help='Dates to be included in consideration')
parser.add_argument('--summarize_strategy_exec', help='Exec used to summarize results')
parser.add_argument('--product_list_file', help='File containing product for which to view summarized '
                                                'result(only used when product=TOTAL)')

args = parser.parse_args()

if args.product:
    product = args.product
else:
    sys.exit('Please provide product for which to summarize result')

if args.strat_folder_or_file:
    strat_folder_or_file = args.strat_folder_or_file
else:
    sys.exit('Please provide folder containing strats or file with strat names')

if args.results_dir:
    results_dir = args.results_dir
else:
    sys.exit('Please provide directory where results are stored')

if args.start_date:
    start_date = args.start_date
else:
    sys.exit('Please provide date from which results are to be considered')

if args.end_date:
    end_date = args.end_date
else:
    sys.exit('Please provide date till which results are to be considered')

if args.skip_dates_file:
    skip_dates_file = args.skip_dates_file

if args.sort_algo:
    sort_algo = args.sort_algo

if args.specific_dates_file:
    specific_dates_file = args.specific_dates_file

if args.summarize_strategy_exec:
    summarize_strategy_exec = args.summarize_strategy_exec

if args.product_list_file:
    product_list_file = args.product_list_file
    
random.seed(time.time())
uniq_id = random.randint(1000000, 9000000)
strat_list_file = "/tmp/strat_list_" + str(uniq_id)

if os.path.isdir(strat_folder_or_file):
    strat_list = os.listdir(strat_folder_or_file)
else:
    with open(strat_folder_or_file) as f:
        strat_list = f.readlines()
    strat_list = [x.strip() for x in strat_list]

if product == "TOTAL":
    with open(product_list_file) as f:
        product_list = f.readlines()
        product_list = [x.strip() for x in product_list]

    product_list = list(set(product_list))
    output_df_ = get_prod_wise_results_for_strats(summarize_strategy_exec, product_list, strat_list, strat_list_file,
                                                  results_dir, start_date, end_date, skip_dates_file, sort_algo,
                                                  specific_dates_file)
    all_df_ = get_results_for_single_prod(summarize_strategy_exec, "ALL", strat_list, strat_list_file,
                                          results_dir, start_date, end_date, skip_dates_file,
                                          sort_algo, specific_dates_file)

    if all_df_.empty:
        print("No results for specified dates")
    else:
        error_log_file_ = os.path.join(results_dir, "error_log")

        error_content = []
        hit_content = []
        sl_content = []

        if os.path.isfile(error_log_file_):
            error_content = [line.strip().split() for line in open(error_log_file_) if 'ERROR' in line]
            hit_content = [line.strip().split() for line in open(error_log_file_) if
                           'HIT' in line and 'STOP LOSS' not in line]
            sl_content = [line.strip().split() for line in open(error_log_file_) if
                          'TENTATIVE' not in line and 'STOP LOSS' in line]

        all_df_['ProdResult'] = all_df_.apply(get_result_string_for_all_prod_,
                                              args=(output_df_, error_content, hit_content, sl_content, start_date,
                                                    end_date), axis=1)
        print("strat prod pnl stdev ab_op_pos vol sharpe med-ttc mkt_pt draw apos msgs pnl_by_loss 95%loss ttv %+vetrds score")
        print((all_df_[['ProdResult']].to_csv(sep=' ', header=False, index=None)).replace('"', ''))

else:
    strat_list_ = [x + "_" + product for x in strat_list]
    with open(strat_list_file, "w") as output:
        output.write('\n'.join(strat_list_))

    output = get_result(summarize_strategy_exec, product, strat_list_file, results_dir, start_date, end_date,
                        skip_dates_file, sort_algo, specific_dates_file, "0")
    print(output)

silent_remove(strat_list_file)
