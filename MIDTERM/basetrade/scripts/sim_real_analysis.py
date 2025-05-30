#!/apps/python2.7/bin/python


import pandas as pd
import numpy as np
import os
import subprocess
import sys
import argparse
import bisect

sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.definitions.execs import paths
from walkforward.definitions.execs import execs


# Process a particular instance in log file to extract following relevant variables :
# Price  : Price of the order
# IntPrice : Price in Tick
# BS : 'B' for buy anfd 'S' for sell
# ST : Current time at which the action is scheduled/executed
# DT : Data time on which action is scheduled (not executed)
# ORR : Action at this instance
# SAOS : ors assigned order sequence
# CAOS : client assigned order seq
# Pos  : current position
# Sz : Size remaining to be executed/canceled
# Sz_exec : Total size executed till now
def ProcessRow(row):
    list_of_words = row.split()
    if len(list_of_words) <= 0 or list_of_words[0] != "SYM:":
        return []
    Price = list_of_words[list_of_words.index("Px:") + 1]
    IntPrice = list_of_words[list_of_words.index("INTPX:") + 1]
    BS = list_of_words[list_of_words.index("BS:") + 1]
    ST = list_of_words[list_of_words.index("ST:") + 1]
    DT = list_of_words[list_of_words.index("DT:") + 1]
    ORR = list_of_words[list_of_words.index("ORR:") + 1]
    SAOS = list_of_words[list_of_words.index("SAOS:") + 1]
    CAOS = list_of_words[list_of_words.index("CAOS:") + 1]
    Pos = list_of_words[list_of_words.index("CLTPOS:") + 1]
    Sz = list_of_words[list_of_words.index("SIZE:") + 1]
    Sz_exec = list_of_words[list_of_words.index("SE:") + 1]
    return [Price, IntPrice, BS, ST, DT, ORR, SAOS, CAOS, Pos, Sz, Sz_exec]


# Generate data for real and sim for a particular query and date
def GenerateData(query_id_, date_):

    # run_accurate_sim_real.pl will provide real log file and sim command (0 flag at the end)
    run_accurate_args_ = [execs().run_accurate_sim_real, date_, query_id_, "0"]

    try:
        run_accurate_out_ = subprocess.check_output(run_accurate_args_, stderr=open(os.devnull, 'w')).split('\n')
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    real_log_file_ = run_accurate_out_[0]

    if not os.path.isfile(real_log_file_ + '.gz'):
        raise OSError('Real log file not found')

    # Extracting SACI corresponding to query
    saci_args_ = ['zgrep', ' SACI:', real_log_file_]
    saci_out_ = subprocess.check_output(saci_args_).split('\n')

    for row in saci_out_:
        words_ = row.split(' ')
        if len(words_) == 0:
            continue
        if words_[-1] == query_id_:
            saci_ = words_[3]
            shc_ = words_[7]

    sim_cmd_args_ = run_accurate_out_[2].split()
    sim_cmd_args_ = sim_cmd_args_[:-1] + ["OM_INFO", "PLSMM_INFO", "SIM_ORDER_INFO", "TRADING_INFO"]

    sim_query_id_ = sim_cmd_args_[3]
    try:
        sim_out_ = subprocess.check_output(sim_cmd_args_, stderr=open(os.devnull, 'w'))
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    # Extracting "SYM:" logs from real and sim log files
    sim_log_file_ = "/spare/local/logs/tradelogs/log." + date_ + "." + sim_query_id_
    sim_order_args_ = ["grep", "SYM", sim_log_file_]

    try:
        sim_order_out_ = subprocess.check_output(sim_order_args_).split('\n')
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    real_order_cmd_ = execs().ors_binary_reader + ' ' + shc_ + ' ' + date_ + ' | grep "SACI: ' + saci_ + '"'

    try:
        real_order_out_ = subprocess.check_output(
            ['bash', '-c', real_order_cmd_], stderr=open(os.devnull, 'w')).split('\n')
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    table_real_ = [ProcessRow(x) for x in real_order_out_]
    table_sim_ = [ProcessRow(x) for x in sim_order_out_]
    data_real_ = pd.DataFrame(table_real_, columns=['Px', 'IntPx', "BS",
                                                    "ST", "DT", "ORR", "SAOS", "CAOS", "Pos", 'Size', 'Sz_Exec'])
    data_sim_ = pd.DataFrame(table_sim_, columns=['Px', 'IntPx', "BS", "ST",
                                                  "DT", "ORR", "SAOS", "CAOS", "Pos", 'Size', 'Sz_Exec'])

    # Since we longer use these in our action logs, we will delete them from out data
    ignorethese = ['Rejc_Funds', 'Wake_Funds', 'ORSConf']
    data_sim_.dropna(inplace=True)
    data_real_.dropna(inplace=True)

    data_real_ = data_real_[~ data_real_['ORR'].str.contains('|'.join(ignorethese))]
    data_sim_ = data_sim_[~ data_sim_['ORR'].str.contains('|'.join(ignorethese))]

    data_real_['ORR'] = data_real_['ORR'].map({'None': 0, 'Seqd': 1, 'Conf': 2, 'CxRe': 3, 'Cxld': 4,
                                               'Cxl': 4, 'Exec': 5, 'IntExec': 5, 'Rejc': 6, 'CxlRejc': 7, 'CxlSeqd': 8, 'CxReRejc': 9})
    data_real_[['Px', 'IntPx', "ST", "DT"]] = data_real_[['Px', 'IntPx', "ST", "DT"]].apply(pd.to_numeric)
    data_real_[["SAOS", "CAOS", "Pos", 'Size', 'Sz_Exec', 'ORR']] = data_real_[
        ["SAOS", "CAOS", "Pos", 'Size', 'Sz_Exec', 'ORR']].astype(int)

    data_sim_ = data_sim_[data_sim_['DT'].apply(is_float)]

    data_sim_['ORR'] = data_sim_['ORR'].map({'None': 0, 'Seqd': 1, 'Conf': 2, 'CxRe': 3, 'Cxld': 4,
                                             'Cxl': 4, 'Exec': 5, 'IntExec': 5, 'Rejc': 6, 'CxlRejc': 7, 'CxlSeqd': 8, 'CxReRejc': 9})
    data_sim_[['Px', 'IntPx', "ST", "DT"]] = data_sim_[['Px', 'IntPx', "ST", "DT"]].apply(pd.to_numeric)
    data_sim_[["SAOS", "CAOS", "Pos", 'Size', 'Sz_Exec', 'ORR']] = data_sim_[
        ["SAOS", "CAOS", "Pos", 'Size', 'Sz_Exec', 'ORR']].astype(int)
    return [data_sim_, data_real_]


def is_float(val):
    try:
        float(val)
    except ValueError:
        return False
    except:
        print(val)
    else:
        return True


def get_utc_date(unix_timestamp):
    '''
    
    Returns the utc time from the unix_time_stamp
    
    unix_timestamp: str
            the unix time to be converted to utc time
    
    returns:
    
    utc_time: str
            the utc time 
    
     
    '''
    process = os.popen("date -d@"+str(unix_timestamp))
    process_out = process.read()
    process.close()
    utc_time=process_out.split()[3]
    return utc_time


def rematch_sim_real_df(real_trade_df,sim_trade_df):
    '''
    
    Find the sim trade , corresponding to the real trade
    
    real_trade_df: pandas dataframe
            the real trades dataframe
    sim_trade_df: pandas dataframe
            the sim trades dataframe
            
        
    returns:
        modified_sim_df: pandas dataframe
            the sim_df matching the real df
        modified_real_df: pandas dataframe
    
     
    '''

    skip_idx_list=[]
    modified_sim_df=pd.DataFrame([],columns=real_trade_df.columns.tolist())
    for index, row in real_trade_df.iterrows():
        data_temp_ = sim_trade_df
        data_temp_ = data_temp_[data_temp_['trade_type'] == row['trade_type']]
        data_temp_ = data_temp_[data_temp_['trade_size'] == row['trade_size']]
        data_temp_ = data_temp_[data_temp_['price'] == row['price']]
        if data_temp_.shape[0]>=1:
            idx=find_closest_idx(data_temp_["time"].as_matrix(),float(row['time']))
            modified_sim_df.loc[index]=sim_trade_df.iloc[idx,:]
        else:
            skip_idx_list.append(index)
    modified_real_trade_df=real_trade_df.drop(real_trade_df.index[[skip_idx_list]])
    return modified_sim_df,modified_real_trade_df


def find_closest_idx(array, value):
    '''
    
    Finds index in the array that is closest to the value
    
    array: numpy array
        The array for which the closest index to the value is to be found
        
    value: float
        The value to search for 
        
    returns:
        idx: int
            the closet index in the array to the value
    
     
    '''
    array = array.astype(float)
    value = float(value)
    temp_array = np.abs((array - value))
    idx = temp_array.argmin() if temp_array.shape[0] > 0 else -1
    return idx


def get_outliers_(points, thresh=10):
    '''
    Gives the boolean numpy array showing the outliers in the array
    
    points: numpy array
            The array where outlier is to be found
    thres: int  default 10
            The z_score threshold to classify as outlier
    
    returns:
        outlier_mask_ : numpy array
            The boolean array denoting the outlier 
     
    '''
    if len(points.shape) == 1:
        points = points[:,None]
    median = np.median(points, axis=0)
    diff = np.sum((points - median)**2, axis=-1)
    diff = np.sqrt(diff)
    med_abs_deviation = np.median(diff)

    modified_z_score = 0.6745 * diff / med_abs_deviation
    outlier_mask_ = modified_z_score > thresh
    return outlier_mask_



def find_sim_real_trade_mismatch_times(sim_trade_df, real_trade_df, mismatch=None):
    '''
    
    Find the times where sim and real pnl are mismatching 
    
    sim_trade_df : pandas dataframe
            The dataframe having sim trades
    
    real_trade_df: pandas dataframe
            The dataframe having real trades
            
    returns:
        sim_time: numpy array
            The array of sim time for mismatch
        real_time: numpy array
            The array real_time for mismatch
    
    
     
    '''
    if sim_trade_df.shape != real_trade_df.shape:
        print ("Mismatch in the number of trades in sim and real!")
        # rematch sim_real trade files
        sim_trade_df, real_trade_df = rematch_sim_real_df(real_trade_df, sim_trade_df)
    sim_slope = (sim_trade_df["real_pnl"].as_matrix()[1:] - sim_trade_df["real_pnl"].as_matrix()[:-1])
    real_slope = (real_trade_df["real_pnl"].as_matrix()[1:] - real_trade_df["real_pnl"].as_matrix()[:-1])
    if mismatch is None:
        mask = get_outliers_((sim_slope - real_slope), 10)
        sim_time = sim_trade_df["time"].as_matrix()[mask + 1]
        real_time = real_trade_df["time"].as_matrix()[mask + 1]
        return sim_time, real_time
    else:
        mask = (np.abs(sim_slope - real_slope)).argsort()[-mismatch:][::-1]
        sim_time = sim_trade_df["time"].as_matrix()[mask + 1]
        print ("%.9f" % sim_time[0])
        real_time = real_trade_df["time"].as_matrix()[mask + 1]
        print ("%.9f" % real_time[0])
        return sim_time, real_time


# For every order sequenced in real, it finds the closet SIM order on same side within permissible time_diff_ and position_diff_ range


def ClosetPoints(row, data,time_diff_,position_diff_):
    data_temp_ = data[data['BS'] == row['BS']]
    data_temp_ = data_temp_[abs(data_temp_['DT'] - row['DT']) <= time_diff_]
    data_temp_ = data_temp_[abs(data_temp_['Pos'] - row['Pos']) <= position_diff_]
    if len(data_temp_.index) > 0:
        row_closet_ = data_temp_.iloc[((data_temp_['DT'] - row['DT']).abs().argsort()[:1])]
        return int(row_closet_['CAOS'])
    else:
        return 0

# Comapres two float values since due to precision error comapring them directly is not correct

def get_real_trade_df(query_id,date):
    year=date[:4]
    month=date[4:6]
    day=date[6:]
    trade_file_directory=os.path.join("/NAS1/logs/QueryTrades/",year,month,day)
    real_trade_file_location=trade_file_directory+"/trades."+date+"."+query_id
    #read the trade file
    return read_trade_file(real_trade_file_location)

def get_sim_trade_df(query_id,date):
    run_accurate_args_ = [execs().run_accurate_sim_real,date,query_id,"0"]
    try:
        run_accurate_out_ = subprocess.check_output(run_accurate_args_, stderr=open(os.devnull, 'w')).split('\n')
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    sim_strategy_command=run_accurate_out_[2]
    sim_prog_id=sim_strategy_command.split()[3]
    sim_trade_file="/spare/local/logs/tradelogs/trades."+date+"."+sim_prog_id


    return read_trade_file(sim_trade_file,True)


def read_trade_file(trade_file_location,sim_file=False):
    with open(trade_file_location) as f:
        lines = f.read().splitlines()

    relevant_list=[elem.split("[")[0] for elem in lines]
    if sim_file==True:
        index = [idx for idx,elem in enumerate(relevant_list) if 'SIMRESULT' in elem]
        temp_list=list(relevant_list[:index[0]])
        relevant_list=list(temp_list)
    #get the time,trade_type,trade_size,price,position,cycle_pnl,real_pnl
    data_matrix=[]
    for elem in relevant_list:
        temp_data=elem.split()
        data_matrix.append([float(temp_data[0]),temp_data[3],int(temp_data[4]),float(temp_data[5]),float(temp_data[6]),float(temp_data[7]),float(temp_data[8])])

    real_trade_df=pd.DataFrame(data_matrix)
    real_trade_df.columns=["time","trade_type","trade_size","price","position","cycle_pnl","real_pnl"]
    return real_trade_df

def isclose(a, b, rel_tol=1e-18, abs_tol=0.000001):
    return abs(a - b) <= max(rel_tol * max(abs(a), abs(b)), abs_tol)


# Find all the details related to a particular order given all data points related to order are provided
def GetAllDetailsFromOrderPoints(data_points_):
    # ST is not taken here as in SIM ST and DT are same while in real ST can be jittery (bsl servers)
    t_seq_ = data_points_.iloc[0]['DT']
    status = 1
    Seq2Conf_ = -1
    CxlSeq2Conf_ = -1
    t_cxl_ = -1
    pos_cxl_ = np.nan  # position at the time cancel action is sequenced
    pos_seq_ = data_points_.iloc[0]['Pos']  # position at the time order is sequenced
    exec_time_list_ = []  # Time list of different executions
    exec_pos_list_ = []  # Size of different executions
    exec_price_list_ = []  # Price of diiferent executions (It can change in case of CancelReplace order)
    total_exec_ = 0
    total_time_order_ = data_points_.iloc[-1]['ST'] - t_seq_
    size_remaining_ = data_points_.iloc[0]['Size']
    data_points_ = data_points_.iloc[1:]

    for index, row in data_points_.iterrows():
        prev_status_ = status
        status = row['ORR']
        if status == 2:  # Conf
            Seq2Conf_ = row['ST'] - t_seq_
            continue
        if status == 3 or status == 9:  # Cancel Modified
            continue
        if status == 8:  # Cxl Seqd
            t_cxl_ = row['DT']
            pos_cxl_ = row['Pos']
        if status == 4:  # Cxl
            CxlSeq2Conf_ = row['ST'] - t_cxl_
            break
        if status == 5:  # Exec
            size_exec_ = size_remaining_ - row['Size']
            size_remaining_ = row['Size']
            exec_price_list_.append("{0:.6f}".format(row['Px']))
            exec_pos_list_.append(int(size_exec_))
            exec_time_list_.append("{0:.6f}".format(row['ST']))
            total_exec_ = row['Sz_Exec']
            if size_remaining_ == 0:
                break
        if status == 6:  # Cxl Reject
            CxlSeq2Conf_ = -1

    return pd.Series([status_vec_[status], t_seq_, int(pos_seq_), Seq2Conf_, t_cxl_, pos_cxl_, CxlSeq2Conf_, total_time_order_, int(total_exec_), int(size_remaining_), exec_time_list_, exec_pos_list_, exec_price_list_])

# Given we have some real orders corresponding to which we have a SIM order with same environment
# within permissoble limit (time_diff and pos_diff), we now try to identify points which diverged


def BehaviorChange(row, sim_data_, real_data_):
    sim_order_points_ = sim_data_[sim_data_['CAOS'] == row['SIM_CAOS']]
    real_order_points_ = real_data_[real_data_['CAOS'] == row['CAOS']]

    # End point of an order will determine its final state (Exception : CancelReplaceReject may come after exec)
    sim_status_ = sim_order_points_.iloc[-1]['ORR']
    real_status_ = real_order_points_.iloc[-1]['ORR']

    sim_end_time_ = sim_order_points_.iloc[-1]['ST']
    real_end_time_ = real_order_points_.iloc[-1]['ST']

    if sim_status_ != real_status_ or ((mode == 1 or mode == 3) and abs(real_end_time_ - sim_end_time_) >= end_time_diff_thresh_):
        sim_order_details_ = GetAllDetailsFromOrderPoints(sim_order_points_)
        real_order_details_ = GetAllDetailsFromOrderPoints(real_order_points_)

        real_pos_at_sim_cancel_time_ = np.nan
        sim_pos_at_real_cancel_time_ = np.nan

        data_sim_exec_ = sim_data_[sim_data_['ORR'] == 4]
        data_real_exec_ = real_data_[real_data_['ORR'] == 4]

        # Here we are calculating the position in Real/SIM when SIM /Real order cancel is sequenced and vice versa
        # They are stored in bracket beside actual position at the time of cancel in SIM/Real
        if sim_order_details_[4] != -1:
            index_prev_real_exec_order_ = bisect.bisect_right(data_real_exec_['ST'].tolist(), sim_order_details_[4])
            if index_prev_real_exec_order_ > 0:
                real_pos_at_sim_cancel_time_ = data_real_exec_.iloc[index_prev_real_exec_order_ - 1]['Pos']
            else:
                real_pos_at_sim_cancel_time_ = 0
        if real_order_details_[4] != -1:
            index_prev_sim_exec_order_ = bisect.bisect_right(data_sim_exec_['ST'].tolist(), real_order_details_[4])
            if index_prev_sim_exec_order_ > 0:
                sim_pos_at_real_cancel_time_ = data_sim_exec_.iloc[index_prev_sim_exec_order_ - 1]['Pos']
            else:
                sim_pos_at_real_cancel_time_ = 0

        sim_order_details_[1] = "{0:.6f}".format(sim_order_details_[1])
        sim_order_details_[4] = "{0:.6f}".format(sim_order_details_[4])
        real_order_details_[1] = "{0:.6f}".format(real_order_details_[1])
        real_order_details_[4] = "{0:.6f}".format(real_order_details_[4])
        # sim_pos_at_real_cxl_time

        sim_order_details_[5] = str(sim_order_details_[5]) + "(" + str(real_pos_at_sim_cancel_time_) + ")"
        real_order_details_[5] = str(real_order_details_[5]) + "(" + str(sim_pos_at_real_cancel_time_) + ")"

        return pd.Series({'Sim_Details': sim_order_details_, 'Real_Details': real_order_details_, 'BS': sim_order_points_.iloc[0]['BS']})
    else:
        return pd.Series({'Sim_Details': [], 'Real_Details': [], 'BS': 0})


query_id_ = ""
date_ = ""
output_file_ = ""
time_diff_ = 0.00002
position_diff_ = 0
end_time_diff_thresh_ = 1
mode = 0
number_mismatch=5

parser = argparse.ArgumentParser()
parser.add_argument('query_id', help='Query ID ')
parser.add_argument('date', help='Date')
parser.add_argument('output_file', help='Path of output file')
parser.add_argument('--time_diff', help='Minimum time difference between sequence time of real and sim order')
parser.add_argument('--pos_diff', help='Minimum position difference at sequence time of real and sim order')
parser.add_argument('--end_time_diff_thresh',
                    help='Minimum time difference between end time of real and sim order allowed')
parser.add_argument('--mode', help=' 0: Only different execution Points 1: Includes Different End time points also 2: Extended output 3: Extended output with additional points (Mode 1+2) 4: Output Info 5: Find fixed number of mismatch')
parser.add_argument('--number_mismatch')

args = parser.parse_args()

if args.query_id:
    query_id_ = args.query_id
else:
    sys.exit('Please provide query ID')

if args.date:
    date_ = args.date
else:
    sys.exit('Please provide date')

if args.output_file:
    output_file_ = args.output_file
else:
    sys.exit('Please provide output file path')

if args.time_diff:
    time_diff_ = float(args.time_diff)

if args.pos_diff:
    position_diff_ = int(args.pos_diff)

if args.end_time_diff_thresh:
    end_time_diff_thresh_ = float(args.end_time_diff_thresh)

if args.mode:
    mode = int(args.mode)

if args.number_mismatch:
    number_mismatch=int(number_mismatch)


[data_sim_, data_real_] = GenerateData(query_id_, date_)
# Vec maintained to map ORR to string
status_vec_ = ['None', 'Seqd', 'Conf', 'CxRe', 'Cxld', 'Exec', 'Rejc', 'CxlRejc', 'CxlSeqd', 'CxReRejc']
data_sim_seqd_ = data_sim_[data_sim_['ORR'] == 1]
data_real_seqd_ = data_real_[data_real_['ORR'] == 1]
data_real_seqd_ = data_real_seqd_.assign(SIM_CAOS="")  # To prevent SetCopy Warning


if mode!=5:

    data_real_seqd_['SIM_CAOS'] = data_real_seqd_.apply(ClosetPoints, args=(data_sim_seqd_,time_diff_,position_diff_), axis=1)

    # Removing rows for which no order was found
    data_real_seqd_ = data_real_seqd_[data_real_seqd_['SIM_CAOS'] != 0]

    # Finding points at which divergence occurs
    divergence_points_ = pd.DataFrame([], columns=['Sim_Details', 'Real_Details', 'BS'])
    divergence_points_ = data_real_seqd_.apply(BehaviorChange, args=(data_sim_, data_real_), axis=1)
    divergence_points_ = divergence_points_[[(len(x) != 0) for x in divergence_points_['Sim_Details']]]

    # Merging real and sim points that diveregd into same row
    divergence_data_ = pd.DataFrame([], columns=['BS', 'S_Status', 'R_Status', 'S_SeqTime', 'R_SeqTime', 'S_PosSeq', 'R_PosSeq', 'S_Seq2Conf', 'R_Seq2Conf', 'S_CxlTime', 'R_CxlTime', 'S_PosCxl', 'R_PosCxl', 'S_CxlSeq2Conf',
                                                 'R_CxlSeq2Conf', 'S_TotalTime', 'R_TotalTime', 'S_TotalExec', 'R_TotalExec', 'S_SzNotExec', 'R_SzNotExec', 'S_ExecTimeList', 'R_ExecTimeList', 'S_ExecPosList', 'R_ExecPosList', 'S_ExecPriceList', 'R_ExecPriceList'])
    i = 0

    for index, row in divergence_points_.iterrows():
        result = [None] * (len(row['Sim_Details']) + len(row['Real_Details']))
        result[::2] = row['Sim_Details']  # Sim Details at odd columns
        result[1::2] = row['Real_Details']  # Real Details at even columns
        result = [row['BS']] + result
        divergence_data_.loc[i] = pd.Series(data=result, index=['BS', 'S_Status', 'R_Status', 'S_SeqTime', 'R_SeqTime', 'S_PosSeq', 'R_PosSeq', 'S_Seq2Conf', 'R_Seq2Conf', 'S_CxlTime', 'R_CxlTime', 'S_PosCxl', 'R_PosCxl', 'S_CxlSeq2Conf',
                                                                'R_CxlSeq2Conf', 'S_TotalTime', 'R_TotalTime', 'S_TotalExec', 'R_TotalExec', 'S_SzNotExec', 'R_SzNotExec', 'S_ExecTimeList', 'R_ExecTimeList', 'S_ExecPosList', 'R_ExecPosList', 'S_ExecPriceList', 'R_ExecPriceList'])
        i += 1

    divergence_data_[['S_PosSeq', 'R_PosSeq', 'S_TotalExec', 'R_TotalExec']] = divergence_data_[
        ['S_PosSeq', 'R_PosSeq', 'S_TotalExec', 'R_TotalExec']].astype(int)
    divergence_data_short_ = divergence_data_[['BS', 'S_Status', 'R_Status', 'S_SeqTime', 'R_SeqTime', 'S_PosSeq', 'R_PosSeq',
                                               'S_CxlTime', 'R_CxlTime', 'S_PosCxl', 'R_PosCxl', 'S_TotalTime', 'R_TotalTime', 'S_TotalExec', 'R_TotalExec']].to_string()

    divergence_data_rest_ = divergence_data_[['S_Seq2Conf', 'R_Seq2Conf', 'S_CxlSeq2Conf', 'R_CxlSeq2Conf',
                                              'S_SzNotExec', 'R_SzNotExec', 'S_ExecTimeList', 'R_ExecTimeList', 'S_ExecPosList', 'R_ExecPosList']].to_string()

    with open(output_file_, "w") as myfile:
        myfile.write(divergence_data_short_)
        myfile.write("\n\n")
        if mode > 1:
            myfile.write(divergence_data_rest_)

    output_info = "\n\n\n\nR_ stands for Real order details and S_ stands for sim order details. \nBS : Whether the order is Buy or Sell\nStatus : Final Status of Order\nSeqTime: Time at which order was sequenced\nPosSeq : Position of query at the time of sequence\nSeq2Conf : Sequenced to confirmation time\nCxlTime : Time at which cancel order was sequenced if sequenced otherwise -1\nPosCxl : Position of query at the time of cancel sequenced (In Bracket : Position at the same time in SIM/Real)\nSeq2Conf : Cancel Sequenced to cancel confirmation time if cancelled\nTotalTime : Total lifetime of the order\nTotal Exec: Total order amount executed\nSzNotExec : Total order amount not executed\nExecTimeList : List of time at which order was executed\nExecPosList : List of partial execution amount corresponding to ExecTimeList\nExecPriceList : List of prices corresponding to ExecTimeList\n";

    with open(output_file_, "a") as myfile:
        myfile.write(output_info)

else:
    sim_trades = get_sim_trade_df(query_id_, date_)
    real_trades = get_real_trade_df(query_id_, date_)
    max_pos_=np.max(np.abs(real_trades["position"].as_matrix()))
    sim_time_array,real_time_array=find_sim_real_trade_mismatch_times(get_sim_trade_df(query_id_, date_),get_real_trade_df(query_id_, date_),number_mismatch)
    sim_trades = get_sim_trade_df(query_id_, date_)
    for time_diff_ in [10,30,100]:
        for position_diff_ in [0,int(max_pos_/2),max_pos_]:
            data_real_seqd_['SIM_CAOS'] = data_real_seqd_.apply(ClosetPoints,
                                                                args=(data_sim_seqd_, time_diff_, position_diff_),
                                                                axis=1)

            # Removing rows for which no order was found
            data_real_seqd_ = data_real_seqd_[data_real_seqd_['SIM_CAOS'] != 0]

            # Finding points at which divergence occurs
            temp_divergence_points_ = pd.DataFrame([], columns=['Sim_Details', 'Real_Details', 'BS'])
            temp_divergence_points_ = data_real_seqd_.apply(BehaviorChange, args=(data_sim_, data_real_), axis=1)
            temp_divergence_points_ = temp_divergence_points_[[(len(x) != 0) for x in temp_divergence_points_['Sim_Details']]]

            # Merging real and sim points that diveregd into same row
            temp_divergence_data_ = pd.DataFrame([],
                                            columns=['BS', 'S_Status', 'R_Status', 'S_SeqTime', 'R_SeqTime', 'S_PosSeq',
                                                     'R_PosSeq', 'S_Seq2Conf', 'R_Seq2Conf', 'S_CxlTime', 'R_CxlTime',
                                                     'S_PosCxl', 'R_PosCxl', 'S_CxlSeq2Conf',
                                                     'R_CxlSeq2Conf', 'S_TotalTime', 'R_TotalTime', 'S_TotalExec',
                                                     'R_TotalExec', 'S_SzNotExec', 'R_SzNotExec', 'S_ExecTimeList',
                                                     'R_ExecTimeList', 'S_ExecPosList', 'R_ExecPosList',
                                                     'S_ExecPriceList', 'R_ExecPriceList'])
            i = 0

            for index, row in temp_divergence_points_.iterrows():
                result = [None] * (len(row['Sim_Details']) + len(row['Real_Details']))
                result[::2] = row['Sim_Details']  # Sim Details at odd columns
                result[1::2] = row['Real_Details']  # Real Details at even columns
                result = [row['BS']] + result
                temp_divergence_data_.loc[i] = pd.Series(data=result,
                                                    index=['BS', 'S_Status', 'R_Status', 'S_SeqTime', 'R_SeqTime',
                                                           'S_PosSeq', 'R_PosSeq', 'S_Seq2Conf', 'R_Seq2Conf',
                                                           'S_CxlTime', 'R_CxlTime', 'S_PosCxl', 'R_PosCxl',
                                                           'S_CxlSeq2Conf',
                                                           'R_CxlSeq2Conf', 'S_TotalTime', 'R_TotalTime', 'S_TotalExec',
                                                           'R_TotalExec', 'S_SzNotExec', 'R_SzNotExec',
                                                           'S_ExecTimeList', 'R_ExecTimeList', 'S_ExecPosList',
                                                           'R_ExecPosList', 'S_ExecPriceList', 'R_ExecPriceList'])
                i += 1

            temp_divergence_data_[['S_PosSeq', 'R_PosSeq', 'S_TotalExec', 'R_TotalExec']] = temp_divergence_data_[
                ['S_PosSeq', 'R_PosSeq', 'S_TotalExec', 'R_TotalExec']].astype(int)

            #get the closest index
            sim_real_mismatch_list=[]
            for real_time in real_time_array:
                real_idx=find_closest_idx(temp_divergence_data_['R_SeqTime'].as_matrix(),real_time)
                sim_real_mismatch_list.append(temp_divergence_data_.as_matrix()[real_idx,:])

            for sim_time in sim_time_array:
                sim_idx=find_closest_idx(temp_divergence_data_["S_SeqTime"].as_matrix(),sim_time)
                sim_real_mismatch_list.append(temp_divergence_data_.as_matrix()[sim_idx,:])

            sim_real_mismatch_df=pd.DataFrame(np.vstack(sim_real_mismatch_list))
            sim_real_mismatch_df.columns=temp_divergence_data_.columns
            sim_real_mismatch_df['R_SeqTime_UTC']=sim_real_mismatch_df['R_SeqTime'].map(get_utc_date)
            sim_real_mismatch_df['S_SeqTime_UTC'] =sim_real_mismatch_df['S_SeqTime'].map(get_utc_date)
            sim_real_mismatch_df=sim_real_mismatch_df[
                    ['BS', 'S_Status', 'R_Status', 'S_SeqTime', 'R_SeqTime', 'S_PosSeq', 'R_PosSeq',
                                               'S_CxlTime', 'R_CxlTime', 'S_PosCxl', 'R_PosCxl', 'S_TotalTime', 'R_TotalTime', 'S_TotalExec', 'R_TotalExec','R_SeqTime_UTC','S_SeqTime_UTC']]

            with open(output_file_, "a") as myfile:
                myfile.write("Sim mismatch for time_diff: "+str(time_diff_)+" position_diff: "+str(position_diff_)+"\n\n\n")
                myfile.write(sim_real_mismatch_df.to_string())
                myfile.write("\n\n")



