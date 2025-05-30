import subprocess
import sys
import os

PSCRIPTS_DIR = "/home/pengine/prod/live_scripts/"
PEXEC_BIN = "/home/pengine/prod/live_execs/"
DELTA_DIR = "/spare/local/logs/pnl_data/hft/delta_files/"
PNL_DIR = "/spare/local/logs/pnl_data/hft/tag_pnl/ind13_pnls/"
GET_SHC_FROM_SYMBOL = PEXEC_BIN+"get_shortcode_for_symbol"
GET_CONTRACT_SPECS = PEXEC_BIN+"get_contract_specs"
GET_SYMBOL_FROM_SHC = PEXEC_BIN+"get_exchange_symbol"
GET_PREV_WEEK_DAY = PEXEC_BIN+"calc_prev_week_day"

date = ""
OVERNIGHT_FILE = ""
EOD_POS_FILE = PNL_DIR+"ind13_pos_"+date+".txt"
EOD_PNL_FILE = PNL_DIR+"ind13_pnl_"+date+".txt"

symbol_info_map = dict()

host_to_process = ['ind-srv13', 'ind-srv15'];

class Trade:
    def __init__(self, trade_str):
        trade_details = trade_str.split(u'\u0001')
        self.symbol = trade_details[0]
        self.buysell = int(trade_details[1])
        self.size = int(trade_details[2])
        self.price = float(trade_details[3])
        self.saos = int(trade_details[4])
        self.ts = float(trade_details[6])
        self.saci = int(trade_details[7])


class SymbolInfo:
    def __init__(self, symbol, **options):
        self.exch_symbol = symbol
        if "shc" in options:
            self.shc = options.get("shc")
        else:
            self.shc = subprocess.check_output([GET_SHC_FROM_SYMBOL, symbol, date]).strip().decode('utf-8')
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
        self.commission = float(subprocess.check_output([GET_CONTRACT_SPECS, self.shc, date, "COMMISH"]).split()[1].strip()) - 0.00003  #Removing Karnataka Stamp charges
        self.intraday_pos = 0
        self.intraday_pnl = 0.0
        self.traded_value = 0.0

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
    symbol_info_map[trade.symbol].intraday_pnl -= trade.size * trade.price * symbol_info_map[trade.symbol].commission

def LoadOvnPosition(filename):
    """
    Expected Ovn File Format
    <Shortcode>,<EOD Positions>,<Last Traded Price>
    :param file:<eod position file>
    :return:void
    """
    try:
        eod_pos_file = open(filename, "r")
        for line in eod_pos_file:
            list = line.split(',')
            shc_ = list[0].strip()
            pos_ = int(list[1].strip())
            lpx_ = float(list[2].strip())
            exch_sym_ = subprocess.check_output([GET_SYMBOL_FROM_SHC, shc_, date]).strip().decode('utf-8')
            if exch_sym_ in symbol_info_map:
                error_string_ = "Error: Two Entries for shortcode "+shc_ +" in eod positions file"
                print(error_string_)
                exit(1)
            else:
                sym_info_ = SymbolInfo(exch_sym_, shc=shc_, eod_pos=pos_, lpx=lpx_)
                symbol_info_map[exch_sym_] = sym_info_
    except OSError:
        print("File Not Found! Continuing with no overnight positions")

def DumpPnls():
    eod_pnl_file = open(EOD_PNL_FILE, "w")
    exchange_ovn_pnl_ = 0.0;
    exch_intraday_pnl_ = 0.0;
    exch_traded_value_ = 0.0;
    for key in symbol_info_map:
        shc_ = symbol_info_map[key].shc
        total_intraday_pnl_ = symbol_info_map[key].intraday_pnl + (symbol_info_map[key].intraday_pos*symbol_info_map[key].lpx) #No commision deducted from open positions
        ovn_pnl_ = symbol_info_map[key].eod_pnl + (symbol_info_map[key].eod_pos*symbol_info_map[key].lpx) #No commision deducted from open positions
        ovn_pos_ = symbol_info_map[key].eod_pos
        intraday_pos_ = symbol_info_map[key].intraday_pos
        traded_val_ = symbol_info_map[key].traded_value
        lpx_ = symbol_info_map[key].lpx

        exchange_ovn_pnl_ += ovn_pnl_
        exch_intraday_pnl_ +=total_intraday_pnl_
        exch_traded_value_ +=traded_val_

        print("| %15s | INTRADAY_PNL: %10.3f | INTRADAY_POS: %6d | OVN_PNL: %10.3f | OVN_POS: %6d | TRADED_VAL: %10d | LPX: %6.3f | TOTAL_PNL: %10.3f | TOTAL_POS: %6d |" %(shc_[4:], total_intraday_pnl_,intraday_pos_,ovn_pnl_,ovn_pos_, traded_val_,lpx_,(total_intraday_pnl_+ovn_pnl_),(intraday_pos_+ovn_pos_)  ), file=eod_pnl_file)
        #eod_pnl_file.write(line+"\n")

    print("\n\n\n| %35s | INTRADAY_PNL: %10.3f | OVN_PNL: %10.3f | TRADED_VAL: %10d | TOTAL_PNL: %10.3f |"% ("NSE", exch_intraday_pnl_,exchange_ovn_pnl_,exch_traded_value_,(exch_intraday_pnl_+ exchange_ovn_pnl_)), file=eod_pnl_file);
    eod_pnl_file.close()

def DumpPositions():

    open_position_list = list()
    for key in symbol_info_map:
        if symbol_info_map[key].eod_pos + symbol_info_map[key].intraday_pos > 0:
            open_position_list.append(symbol_info_map[key].shc + ", " + str(symbol_info_map[key].eod_pos + symbol_info_map[key].intraday_pos)+", "+str(symbol_info_map[key].lpx))
    open_position_list.sort()
    eod_pos_file = open(EOD_POS_FILE, "w")
    for line in open_position_list:
        eod_pos_file.write(line+"\n")
    eod_pos_file.close()

def main():
    global date
    global EOD_POS_FILE, EOD_PNL_FILE
    USAGE="Usage: python <script> <date> <trade_file-optional>"
    trade_files_ = list()
    if len(sys.argv) < 2:
       print(USAGE, file=sys.stderr)
    elif len(sys.argv) == 2:
        date = sys.argv[1]
        file_list = os.listdir(DELTA_DIR)
        for fname in file_list:
            if any(x in fname for x in host_to_process):
                complete_filename = DELTA_DIR+fname
                trade_files_.append(complete_filename)

    elif len(sys.argv) == 3:
        date = sys.argv[1]
        trade_files_.append(sys.argv[2])
    else:
       print(USAGE, file=sys.stderr)

    EOD_POS_FILE = PNL_DIR + "ind13_pos_" + date + ".txt"
    EOD_PNL_FILE = PNL_DIR + "ind13_pnl_" + date + ".txt"

    prev_day = subprocess.check_output([GET_PREV_WEEK_DAY, date]).strip().decode('utf-8')
    OVERNIGHT_FILE = PNL_DIR +"ind13_pos_"+prev_day+".txt"
    LoadOvnPosition(OVERNIGHT_FILE)

    for file in trade_files_:
        try:
            trade_file = open(file, "r")
            for line in trade_file:
                this_trade_ = Trade(line)
                processTrade(this_trade_)
        except OSError:
            print("Trade File: " + file + "Not Found!")

    DumpPnls()
    DumpPositions()
if __name__ == "__main__": main()



