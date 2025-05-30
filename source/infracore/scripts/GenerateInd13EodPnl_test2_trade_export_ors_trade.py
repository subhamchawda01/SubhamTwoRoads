import subprocess
import sys
import os
import argparse
import configparser
import paramiko
import errno
import shutil
import re
import trlibs

#from statsmodels.sandbox.regression import penalized

date = ""
exch_dir=""
OVERNIGHT_FILE = ""
EXCHANGE = "NSE"
TMP_DIR = "/tmp/ind_pnls/" #This is the directory for doing storing local files
TMP_TRADE_DIR=TMP_DIR+"trades/"
TMP_OUT_DIR=TMP_DIR+"out/"
TMP_PNL_DIR=TMP_OUT_DIR+"eod_pnls/"
TMP_POS_DIR=TMP_OUT_DIR+"eod_positions/"


NAS_OUT_DIR_REMOTE="/NAS1/data/MFGlobalTrades/ind_pnls/" #Use this path while remotely accessing the drive
NAS_OUT_DIR_NY="/NAS1/data/MFGlobalTrades/ind_pnls/" #Use this path for reading only
NAS_NEAT_TRADE_DIR_REMOTE=NAS_OUT_DIR_REMOTE+exch_dir+"/" #Append dir argument from the script

PSCRIPTS_DIR = "/home/pengine/prod/live_scripts/"
PEXEC_BIN = "/home/pengine/prod/live_execs/"
PCONFIG = "/home/pengine/prod/live_configs/"

CONFIG_FILE = PCONFIG+"ind_pnl_config.ini"

PNL_DIR_NY = NAS_OUT_DIR_NY+"eod_pnls/"
POS_DIR_NY =NAS_OUT_DIR_NY+"eod_positions/"
NEAT_TRADES_DIR_NY=NAS_OUT_DIR_NY+exch_dir+"neat_trades/"
PENALITY_DIR_NY=NAS_OUT_DIR_NY+exch_dir+"penalities/"

PNL_DIR_REMOTE = NAS_OUT_DIR_NY+"eod_pnls/"
POS_DIR_REMOTE = NAS_OUT_DIR_NY+"eod_positions/"


GET_PREV_WORKING_DAY = PEXEC_BIN+"calc_prev_working_day"
BULK_CONTRACT_SPECS = PEXEC_BIN+"get_nse_commission"
#BULK_CONTRACT_SPECS = "/home/dvcinfra/trash/hardik/get_nse_commission"
#BULK_CONTRACT_SPECS = "/home/dvcinfra/trash/hardik/get_nse_commission_20200630"

EOD_INTRADAY_POS_FILE = TMP_POS_DIR +"ind_intraday_pos_"+date+".txt"
EOD_POS_FILE = TMP_POS_DIR +"ind_pos_"+date+".txt"
EOD_PNL_FILE = TMP_PNL_DIR+"ind_pnl_"+date+".txt"

EOD_INTRADAY_POS_FILE_NAS=POS_DIR_REMOTE+"ind_intraday_pos_"+date+".txt"
EOD_POS_FILE_NAS=POS_DIR_REMOTE+"ind_pos_"+date+".txt"
EOD_PNL_FILE_NAS=PNL_DIR_REMOTE+"ind_pnl_"+date+".txt"

ALL_EXCH_SYM_FILE = "/tmp/exch_sym_in."+date #Exchange symbols present in overnight positions and trade files
COMMISSION_FILE = "/tmp/commission."+date    #Stores the exchange symbol shc and commission of products in
NEAT_TRADE_FILE = NEAT_TRADES_DIR_NY+"trades."+date #This path is incorrect, This path is set in the main()
PENALITY_FILE = PENALITY_DIR_NY+"penality."+date
NAS_IP="10.23.5.13"
INFRA_USER="dvcinfra"
MIDTERM_DB="/spare/local/tradeinfo/NSE_Files/midterm_db"

# ALL_EXCH_SYM_FILE
symbol_info_map = dict()
trans_add_symb = set()

class Trade:
    @classmethod
    def fill_from_ors_trade_line(cls, trade_str):
        trade_details = trade_str.split(u'\u0001')
        trade = cls()
        trade.symbol = trade_details[0]
        trade.buysell = int(trade_details[1])
        trade.size = int(trade_details[2])
        trade.price = float(trade_details[3])
        trade.saos = int(trade_details[4])
        trade.ts = float(trade_details[6])
        trade.saci = int(trade_details[7])
        return trade

    @classmethod
    def fill_from_neat_trade_line(cls, neat_trade_str):
        #TODO Verify this
        trade = cls()
        neat_trade_details = neat_trade_str.split(',')
        trade.symbol = "NSE_"+neat_trade_details[4].strip()
        trade.buysell = int(0 if neat_trade_details[9] =='B' else 1)
        trade.size = int(neat_trade_details[6])
        trade.price = float(neat_trade_details[7])
        trade.saos = int(neat_trade_details[10])
        trade.ts = float(neat_trade_details[2])
        trade.saci = 123456
        return trade


class SymbolInfo:
    def __init__(self, symbol, **options):
        self.exch_symbol = symbol
        if "shc" in options:
            self.shc = options.get("shc")
        else:
            self.shc = subprocess.check_output(["grep", "-w", self.exch_symbol, COMMISSION_FILE]).split()[1].strip().decode('utf-8')
            if self.shc == '1e+06':
                print("Expired Product: "+self.exch_symbol)
                #TODO

        if "lpx" in options:
            self.lpx = float(options.get("lpx"))
        else:
            self.lpx = 0.0
        if "eod_pos" in options:
            self.eod_pos = int(options.get("eod_pos"))
            self.eod_pnl = (-1) * self.eod_pos * self.lpx
        else:
            self.eod_pos = 0
            self.eod_pnl = 0.0
        self.commission = float(subprocess.check_output(["grep", self.exch_symbol, COMMISSION_FILE]).split()[
                                    2].strip())
        # Removing Karnataka Stamp charges
        if( exch_dir == "CM"):
            self.lotsize = 1
        else:
            self.lotsize = int(LotSize(self.shc[4:]))
        self.intraday_pos = 0
        self.intraday_pnl = 0.0
        self.traded_value = 0.0
        self.trans_comm_add = 0.0


def get_sftp_client(user, server_ip):
    #Creates and returns sftp client objects
    ssh_client = paramiko.SSHClient()
    ssh_client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh_client.connect(hostname=server_ip, username=user)
    return ssh_client.open_sftp()


def mkdir_p(path):
    #Function to emulate mkdir -p in shell
    try:
        os.makedirs(path)
    except OSError as exc:
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else:
            raise


def mkdir_p_remote(remote_directory, sftp_client):
    #Create directories recursively if not exists on remote machines
    """Change to this directory, recursively making new folders if needed.
    Returns True if any folders were created."""
    if remote_directory == '/':
        # absolute path so change directory to root
        sftp_client.chdir('/')
        return
    if remote_directory == '':
        # top-level relative directory must exist
        return
    try:
        sftp_client.chdir(remote_directory) # sub-directory exists
    except IOError:
        dirname, basename = os.path.split(remote_directory.rstrip('/'))
        mkdir_p_remote(dirname, sftp_client) # make parent directories
        sftp_client.mkdir(basename) # sub-directory missing, so created it
        sftp_client.chdir(basename)
        return True


def process_neat_trades():
    try:
        neat_trade_file = open(NEAT_TRADE_FILE,'r')
        next(neat_trade_file) #Skip first line as it contains headers
        for line in neat_trade_file:
            trade = Trade.fill_from_neat_trade_line(line)
            processTrade(trade)
    except OSError:
        print("No neat trades found ["+NEAT_TRADE_FILE+"]")
        return


def get_penality():
    try:
        penality_file = open(PENALITY_FILE,'r')
        for line in penality_file:
            return float(str(line).strip()) #TODO
    except OSError:
        print("No penality file found ["+PENALITY_FILE+"]")
        return 0

def processTrade(trade):
    if trade.symbol not in symbol_info_map:
        sym_info_ = SymbolInfo(trade.symbol)
        symbol_info_map[trade.symbol] = sym_info_

    if trade.buysell == 0:
        #Buy
        symbol_info_map[trade.symbol].intraday_pos += trade.size
        symbol_info_map[trade.symbol].intraday_pnl -= trade.size * trade.price
    else:
        symbol_info_map[trade.symbol].intraday_pos -= trade.size
        symbol_info_map[trade.symbol].intraday_pnl += trade.size * trade.price

    symbol_info_map[trade.symbol].lpx = trade.price
    symbol_info_map[trade.symbol].traded_value += trade.size * trade.price
 
    if exch_dir == "CM" and trade.symbol not in trans_add_symb:
      symbol_info_map[trade.symbol].trans_comm_add += 0.000005 * trade.size * trade.price 
    else:
      #print("ignore SYMB found: "+trade.symbol)
      symbol_info_map[trade.symbol].trans_comm_add = 0

    symbol_info_map[trade.symbol].intraday_pnl -= trade.size * trade.price * symbol_info_map[trade.symbol].commission

def LoadOvnPosition(filename):
    """
    Expected Ovn File Format
    <Exchange Symbol>,<EOD Positions>,<Last Traded Price>
    :param file:<eod position file>
    :return:void
    """
    try:
        eod_pos_file = open(filename, "r")
        for line in eod_pos_file:
            exch_list = line.split(',')
            exch_sym_ = exch_list[0].strip()
            pos_ = int(exch_list[1].strip())
            if (exch_dir == "CM" and pos_ < 0):
                pos_ = 0
            lpx_ = float(exch_list[2].strip())
            if exch_sym_ in symbol_info_map:
                error_string_ = "Error: Two Entries for symbol "+ exch_sym_+" in eod positions file"
                print(error_string_)
                exit(1)
            else:
                if subprocess.check_output(["grep", exch_sym_, COMMISSION_FILE]).split()[1].strip().decode(
                        'utf-8') != '1e+06': #Check for Expired Products
                    sym_info_ = SymbolInfo(exch_sym_, eod_pos=pos_, lpx=lpx_)
                    symbol_info_map[exch_sym_] = sym_info_
    except OSError:
        print("File Not Found["+filename+"]! Continuing with no overnight positions")

def LotSize(symbol):
    global MIDTERM_DB
    try:
        if(symbol.find("_FUT") != -1):
            pos = symbol.find("_FUT")
            fut = symbol[pos+4:pos+5]
            stock = symbol[:pos]
        else:
            pos = re.search("_[CP][01]_",symbol).start()
            fut = symbol[pos+2:pos+3]
            stock = symbol[:pos]
        cmd = "sqlite3 " + MIDTERM_DB + " 'select lotsize from LOTSIZES where day=" + str(date) + " and stock=\"" + stock + "\" and expnum=" + fut + "';";
        lotsize = subprocess.check_output(cmd, shell=True);
        return lotsize
    except:
        return 1

def DumpPnls():
    eod_pnl_file = open(EOD_PNL_FILE, "w")
    exchange_ovn_pnl_ = 0.0;
    exch_intraday_pnl_ = 0.0;
    exch_traded_value_ = 0.0;
    for key in symbol_info_map:
        exch_sym_ = key;
        shc_ = symbol_info_map[key].shc

        total_intraday_pnl_ = symbol_info_map[key].intraday_pnl + (symbol_info_map[key].intraday_pos*symbol_info_map[key].lpx) #No commision deducted from open positions
        total_intraday_pnl_before_trans_comm_add = total_intraday_pnl_
        total_intraday_pnl_ += symbol_info_map[key].trans_comm_add 


        ovn_pnl_ = symbol_info_map[key].eod_pnl + (symbol_info_map[key].eod_pos*symbol_info_map[key].lpx) #No commision deducted from open positions
        ovn_pos_ = symbol_info_map[key].eod_pos
        intraday_pos_ = symbol_info_map[key].intraday_pos
        traded_val_ = symbol_info_map[key].traded_value
        lpx_ = symbol_info_map[key].lpx

        exchange_ovn_pnl_ += ovn_pnl_
        exch_intraday_pnl_ += total_intraday_pnl_
        exch_traded_value_ += traded_val_
        no_of_lot = (intraday_pos_+ovn_pos_)/symbol_info_map[key].lotsize

        if exch_dir == "CM" and key not in trans_add_symb:
          prev_total_pnl = total_intraday_pnl_before_trans_comm_add+ovn_pnl_
          curr_total_pnl = total_intraday_pnl_+ovn_pnl_
          print("SYMB found: "+key+": trans_comm_added, Prev_total_pnl, cur_total_pnl:: "+str(symbol_info_map[key].trans_comm_add)+", "+str(prev_total_pnl)+", "+str(curr_total_pnl))


        print("| %18s | INTRADAY_PNL: %10.3f | INTRADAY_POS: %6d | OVN_PNL: %10.3f | OVN_POS: %6d | TRADED_VAL: %10d "
              "| LPX: %9.3f | TOTAL_PNL: %10.3f | TOTAL_POS: %6d | NO_OF_LOT: %6d | %15s |" %(shc_[4:],total_intraday_pnl_,intraday_pos_,
                                                                        ovn_pnl_,ovn_pos_,traded_val_,lpx_,
                                                                             (total_intraday_pnl_+ovn_pnl_),
                                                                             (intraday_pos_+ovn_pos_),no_of_lot, exch_sym_ ),
              file=eod_pnl_file)
        #eod_pnl_file.write(line+"\n")
    penality = get_penality()
    print("\n\n\n| %35s | INTRADAY_PNL: %10.3f | OVN_PNL: %10.3f | PENALITY: %10.3f |TRADED_VAL: %10d | TOTAL_PNL: "
          "%10.3f |"% (
        "NSE",
                                                                                                              exch_intraday_pnl_,exchange_ovn_pnl_, penality, exch_traded_value_,(exch_intraday_pnl_+ exchange_ovn_pnl_-penality)), file=eod_pnl_file);
    eod_pnl_file.close()

def DumpPositions():

    open_position_list = list()
    for key in symbol_info_map:
        if (exch_dir == "FO" and (symbol_info_map[key].eod_pos + symbol_info_map[key].intraday_pos) != 0) or (exch_dir == "CM" and (symbol_info_map[key].eod_pos + symbol_info_map[key].intraday_pos) != 0):
            open_position_list.append(key + ", " + str(symbol_info_map[key].eod_pos + symbol_info_map[
                key].intraday_pos)+", "+str(symbol_info_map[key].lpx))
    open_position_list.sort()
    eod_pos_file = open(EOD_POS_FILE, "w")
    for line in open_position_list:
        eod_pos_file.write(line+"\n")
    eod_pos_file.close()

def DumpIntradayPositions():

    open_position_list = list()
    for key in symbol_info_map:
        if (exch_dir == "FO" and (symbol_info_map[key].intraday_pos) != 0) or (exch_dir == "CM" and (symbol_info_map[key].intraday_pos) != 0):
            open_position_list.append(key + ", " + str(symbol_info_map[
                key].intraday_pos)+", "+str(symbol_info_map[key].lpx))
    open_position_list.sort()
    eod_pos_file = open(EOD_INTRADAY_POS_FILE, "w")
    for line in open_position_list:
        eod_pos_file.write(line+"\n")
    eod_pos_file.close()

def backup_eod_to_nas():
    #sftp_client = get_sftp_client(INFRA_USER,NAS_IP)
    shutil.copyfile(EOD_PNL_FILE, EOD_PNL_FILE_NAS)
    shutil.copyfile(EOD_POS_FILE, EOD_POS_FILE_NAS)
    shutil.copyfile(EOD_INTRADAY_POS_FILE, EOD_INTRADAY_POS_FILE_NAS)
    #sftp_client.close

def cleanup():
    shutil.rmtree(TMP_TRADE_DIR)
    shutil.rmtree(TMP_POS_DIR)
    shutil.rmtree(TMP_PNL_DIR)


def update_file_paths():
    global date, exch_dir
    global EOD_POS_FILE, EOD_INTRADAY_POS_FILE, EOD_PNL_FILE, COMMISSION_FILE, ALL_EXCH_SYM_FILE, PNL_DIR_REMOTE, POS_DIR_REMOTE, \
        PNL_DIR_NY, POS_DIR_NY, TMP_PNL_DIR, TMP_POS_DIR, TMP_TRADE_DIR, EOD_POS_FILE_NAS, EOD_PNL_FILE_NAS, \
        EOD_INTRADAY_POS_FILE_NAS, NAS_NEAT_TRADE_DIR_REMOTE, NEAT_TRADE_FILE, NEAT_TRADES_DIR_NY, OVERNIGHT_FILE, PENALITY_DIR_NY, PENALITY_FILE

    # Update the path for eod_pnl and eod_positions file
    TMP_PNL_DIR = TMP_OUT_DIR + exch_dir + "/eod_pnls/"
    TMP_POS_DIR = TMP_OUT_DIR + exch_dir + "/eod_positions/"
    TMP_TRADE_DIR = TMP_TRADE_DIR + exch_dir + "/"

    PNL_DIR_REMOTE = NAS_OUT_DIR_NY + exch_dir + "/eod_pnls/"
    POS_DIR_REMOTE = NAS_OUT_DIR_NY + exch_dir + "/eod_positions/"
    NAS_NEAT_TRADE_DIR_REMOTE = NAS_OUT_DIR_REMOTE + exch_dir + "/neat_trades/"

    PNL_DIR_NY = NAS_OUT_DIR_NY + exch_dir + "/eod_pnls/"
    POS_DIR_NY = NAS_OUT_DIR_NY + exch_dir + "/eod_positions/"
    NEAT_TRADES_DIR_NY = NAS_OUT_DIR_NY + exch_dir + "/neat_trades/"
    PENALITY_DIR_NY = NAS_OUT_DIR_NY + exch_dir + "/penalities/"

    NEAT_TRADE_FILE = NEAT_TRADES_DIR_NY + "trades." + str(date)
    PENALITY_FILE = PENALITY_DIR_NY + "penality."+ str(date)

    EOD_INTRADAY_POS_FILE = TMP_POS_DIR + "/ind_intraday_pos_" + str(date) + ".txt"
    EOD_POS_FILE = TMP_POS_DIR + "/ind_pos_" + str(date) + ".txt"
    EOD_PNL_FILE = TMP_PNL_DIR +"/ind_pnl_" + str(date) + ".txt"

    EOD_INTRADAY_POS_FILE_NAS=POS_DIR_REMOTE+"ind_intraday_pos_"+str(date)+".txt"
    EOD_POS_FILE_NAS=POS_DIR_REMOTE+"ind_pos_"+str(date)+".txt"
    EOD_PNL_FILE_NAS=PNL_DIR_REMOTE+"ind_pnl_"+str(date)+".txt"
    ALL_EXCH_SYM_FILE = "/tmp/exch_sym_in." + str(date)
    COMMISSION_FILE = "/tmp/commission." + str(date)
    prev_day = trlibs.GetPreviousWorkingDay(str(date), "NSE");
    OVERNIGHT_FILE = POS_DIR_NY +"ind_pos_"+prev_day+".txt"


def create_directories_if_not_exists():
    #Create all temporay directories if not exists
    mkdir_p(TMP_PNL_DIR)
    mkdir_p(TMP_POS_DIR)
    mkdir_p(TMP_TRADE_DIR)

    '''Create directories on NAS if not exists'''
    #sftp_client = get_sftp_client(INFRA_USER, NAS_IP)
    mkdir_p(PNL_DIR_REMOTE)
    mkdir_p(POS_DIR_REMOTE)
    mkdir_p(NAS_NEAT_TRADE_DIR_REMOTE)
    #sftp_client.close()


def main():
    global date, exch_dir #Update these using arguments to the string

    '''Parse Config file to get list of supported servers and their profiles'''
    config = configparser.ConfigParser()
    config.read(CONFIG_FILE)

    '''Parse the command line arguments'''
    parser = argparse.ArgumentParser(
        description='''Computes the end of day pnls, open positions for NSE products.''',
    )

    parser.add_argument('-D', '--dir', action="store", default="", type=str,
                        help='<Required> Give name of directory where '
                             'you want to store the generated files',
                        required=True)
    parser.add_argument('-t', '--trade_file', action="store",default="",type=str, help='<Required>  containing the ors trades. ',
                        required=True)
    parser.add_argument('-d', '--date', action="store", dest="date", type=int,
                        help='<Required> Date for which the EOD pnl '
                             'is being calculated. Used for fetching '
                             'the shortcodes, commission and '
                             'settlement prices.', required=True)

    args = parser.parse_args() #Stores all the commandline arguments
    date = args.date
    exch_dir = args.dir

    update_file_paths()
    #cleanup()
    create_directories_if_not_exists()

    '''Fetch all the trades from trade files'''
    trade_files_ = list()
    trade_files_.append(args.trade_file)
    '''
    add trade file to adjust pos
    '''
    TRADE_FILE_TO_ADJUST_POS=NAS_OUT_DIR_NY + '' + exch_dir + "/trades_to_adjust/trades." + str(date)
    if os.path.isfile(TRADE_FILE_TO_ADJUST_POS):
        trade_files_.append(TRADE_FILE_TO_ADJUST_POS)
    else:
        print("Tradefile not found. Filename: "+ TRADE_FILE_TO_ADJUST_POS);

    TRANS_ADD_SYM_FILE="/tmp/data_nifty_next." + str(date)
    trans_add_sym_file = open(TRANS_ADD_SYM_FILE, "r")
    try:
      for line in trans_add_sym_file:
        symb = "NSE_"+line.strip()
        if symb not in trans_add_symb:
#          print("trans sym in "+symb)
          trans_add_symb.add(symb)
    except OSError:
      print("File Not Found ["+TRANS_ADD_SYM_FILE+"]! Continuing with no trasnaction_add_sym positions")


    exch_set = set()
    '''Add symbols from overnight file'''
    try:
        eod_pos_file = open(OVERNIGHT_FILE, "r")
        for line in eod_pos_file:
            exch_list = line.split(',')
            exch_sym_ = exch_list[0].strip()
            if exch_sym_ not in exch_set:
                exch_set.add(exch_sym_)
    except OSError:
        print("File Not Found ["+OVERNIGHT_FILE+"]! Continuing with no overnight positions")

#Add symbols for neat trades
    try:
        neat_pos_file = open(NEAT_TRADE_FILE, "r")
        for line in neat_pos_file:
            exch_list = line.split(',')
            exch_sym_ = "NSE_"+exch_list[4].strip()
            if exch_sym_ not in exch_set:
                exch_set.add(exch_sym_)
    except OSError:
        print("File Not Found ["+NEAT_TRADE_FILE+"]! Continuing with no neat trades")
    #Add symbols from trade files
    for file in trade_files_:
        print("TradeFILE: " + file + " ")
        try:
            trade_file = open(file, "r")
            for line in trade_file:
                this_trade_ = Trade.fill_from_ors_trade_line(line)
                if this_trade_.symbol not in exch_set:
                    exch_set.add(this_trade_.symbol)
        except OSError:
            print("Trade File: " + file + "Not Found!")

    exch_sym_file = open(ALL_EXCH_SYM_FILE, "w")
    for sym in exch_set:
        exch_sym_file.write(sym+"\n")
    exch_sym_file.close()
    cmd = BULK_CONTRACT_SPECS + " " + str(date) + " "+ exch_dir + " " + ALL_EXCH_SYM_FILE+ " " + COMMISSION_FILE;
    ret_code = subprocess.check_output(cmd, shell=True);
    #cmd = "cat "+ALL_EXCH_SYM_FILE+";"+BULK_CONTRACT_SPECS+" " +ALL_EXCH_SYM_FILE+" " \
     #   + COMMISSION_FILE + " " + str(date)
    #ret_code = subprocess.check_output(cmd,shell=True)

    LoadOvnPosition(OVERNIGHT_FILE)
    for file in trade_files_:
        try:
            trade_file = open(file, "r")
            for line in trade_file:
                this_trade_ = Trade.fill_from_ors_trade_line(line)
                processTrade(this_trade_)
        except OSError:
            print("Trade File: " + file + "Not Found!")

    process_neat_trades()
    DumpPnls()
    DumpPositions()
    DumpIntradayPositions()

    backup_eod_to_nas()
    cleanup()
if __name__ == "__main__": main()
