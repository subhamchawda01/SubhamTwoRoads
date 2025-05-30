import sys
import argparse
from get_pnl_stats import create_stats_from_content
import pandas as pd
import io


def create_multiple_stats_from_content(content):

    trades_content_ = [x for x in content if "SIMRESULT" not in x and "PORTRESULT" not in x and "TOTALRESULT" not in x]
    sim_result_content_ = [x for x in content if "SIMRESULT" in x ]
    port_result_content_ = [x for x in content if "PORTRESULT" in x]
    total_result_content_ = [x for x in content if "TOTALRESULT" in x]

    try:
        trades_ = pd.read_csv(io.StringIO('\n'.join(trades_content_)),
                              delim_whitespace=True, header=None, error_bad_lines=False)
        trades_['Name'], trades_['ID'] = trades_[2].str.split('.', 1).str
    except Exception as e:
        trades_ = pd.DataFrame()
        print(e)
        # print(date,e)
        # file = "/spare/local/logs/tradelogs/error"+date
        # fh = open(file,"w")
        # fh.writelines(trades_content_)
        # fh.close()
        return []

    pnl_stats_ = []

    try:
        sim_results_ = pd.read_csv(io.StringIO('\n'.join(sim_result_content_)), delim_whitespace=True,
                                   header=None, error_bad_lines=False)
        sim_results_.set_index(1, inplace=True)
        gb = trades_.groupby('Name')
        for key, x in gb:
            sim_result = sim_results_.loc[key].tolist()
            pnl_stats_list_ = create_stats_from_content(x.values.tolist(), 1).split()

            pnl_stats_list_.insert(12,sim_result[10]) # Number of messages
            pnl_stats_list_.insert(14, sim_result[8])  # Market Volume Traded
            pnl_stats_list_.insert(16, 0) #UTS

            pnl_stats_list_[8] = sim_result[9]  # min pnl
            pnl_stats_list_[13] = sim_result[7]  # exposure

            pnl_stats_.append([key] + sim_result[1:7] + pnl_stats_list_)
    except Exception as e:
        #print(e)
        pass

    try:
        port_results_ = pd.read_csv(io.StringIO('\n'.join(port_result_content_)),
                                    delim_whitespace=True, header=None, error_bad_lines=False)
        port_results_['Name'], port_results_['ID'] = port_results_[1].str.split('.', 1).str
        port_results_.set_index(1, inplace=True)
        port_results_.set_index('ID', inplace=True)
        gb = trades_.groupby('ID')
        for key, x in gb:
            port_result = port_results_.loc[key].tolist()
            pnl_stats_list_ = create_stats_from_content(x.values.tolist(), 1, 17, 16).split()

            pnl_stats_list_.insert(12,0) # Number of messages
            pnl_stats_list_.insert(14, port_result[8])  # Market Volume Traded
            pnl_stats_list_.insert(16, 0) #UTS

            pnl_stats_list_[8] = port_result[9]  # min pnl
            pnl_stats_list_[13] = port_result[7]  # exposure

            pnl_stats_.append([port_result[10]] + port_result[1:7] + pnl_stats_list_)
    except Exception as e:
        #print(e)
        pass

    try:
        for line in total_result_content_:
            pnl_stats_list_ = [0]*20
            total_result_ = line.split()[1:]

            pnl_stats_list_[8] = total_result_[9]  # min pnl
            pnl_stats_list_[12] = total_result_[10] # Number of messages
            pnl_stats_list_[13] = total_result_[7]  # exposure
            pnl_stats_list_[14] = total_result_[8]  # MarketVolume traded

            pnl_stats_.append(total_result_[0:7] + pnl_stats_list_)
    except Exception as e:
        print(e.message)
        # print(date,e.message)
        # file = "/spare/local/logs/tradelogs/error"+date
        # fh = open(file,"w")
        # fh.writelines(trades_content_)
        # fh.close()
        pass

    return pnl_stats_

if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument('trades_file', help='Trades files for multiple product')

    args = parser.parse_args()

    if args.trades_file:
        trades_file = args.trades_file
    else:
        sys.exit('Please provide input strat folder')

    with open(trades_file) as f:
        content_ = f.readlines()
    # you may also want to remove whitespace characters like `\n` at the end of each line
    content_ = [y.strip() for y in content_]

    pnl_stats = create_multiple_stats_from_content(content_)

    for x in pnl_stats:
        print(' '.join(map(str, x)))
