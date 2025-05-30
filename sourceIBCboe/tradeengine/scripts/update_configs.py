
import pandas as pd
import os
import argparse
import sys
import shutil
from prod_stats import ProdStats
from create_strat_permutations import copytree

def changeMainConfig(config_folder, prod_stats, offset_spread_factor,
                     min_offset, max_trade_value, start_date_, end_date_, stats_folder_):

    try:
        data = pd.read_csv(os.path.join(config_folder, "MainConfig.cfg"), delim_whitespace=True, header=None)
    except IOError:
        return

    data.set_index(0, inplace=True)

    primary_prod = data.loc["PRIMARY0", 2]
    secondary_prod = data.loc["SECONDARY", 2]

    if primary_prod not in prod_stats.keys():
        prod_stats[primary_prod] = ProdStats(primary_prod, start_date_, end_date_, stats_folder_).stats

    if secondary_prod not in prod_stats.keys():
        prod_stats[secondary_prod] = ProdStats(secondary_prod, start_date_, end_date_, stats_folder_).stats

    primary_stats = prod_stats[primary_prod]
    secondary_stats = prod_stats[secondary_prod]

    offset = max((secondary_stats.AvgSpd*0.05*100/secondary_stats.Px)*offset_spread_factor, min_offset)
    max_primary_spread = primary_stats.AvgSpd*0.05*10
    max_secondary_spread = secondary_stats.AvgSpd * 0.05 * 10
    pos = int(min(secondary_stats.L1Sz*secondary_stats.Px, max_trade_value)/secondary_stats.Px)

    token = config_folder.split("_")
    if token[-1] != "SQUAREOFF":
        data.loc["BID_PERCENTAGE_OFFSET", 2] = offset
        data.loc["ASK_PERCENTAGE_OFFSET", 2] = offset

        num_shifts = int(data.loc["BID_INCREASE_MAX_SHIFT", 2])
        data.loc["BID_INCREASE_SHIFT_PERCENT", 2] = offset / num_shifts
        data.loc["BID_DECREASE_SHIFT_PERCENT", 2] = offset / num_shifts
        data.loc["ASK_INCREASE_SHIFT_PERCENT", 2] = offset / num_shifts
        data.loc["ASK_DECREASE_SHIFT_PERCENT", 2] = offset / num_shifts

    data.loc["POSITION_SHIFT_AMOUNT", 2] = pos
    data.loc["MAX_PRIMARY_SPREAD", 2] = max_primary_spread
    data.loc["MAX_SECONDARY_SPREAD", 2] = max_secondary_spread

    data.to_csv(os.path.join(config_folder, "MainConfig.cfg"), index=True, header=None, sep=" ")

    return primary_stats, secondary_stats, offset, pos, secondary_prod


# Size changes will be done in every executioner simultaneously using sed command

def changeDimerFile(config_folder, secondary_stats, pos, max_offset_fraction):

    try:
        data = pd.read_csv(os.path.join(config_folder, "Dimer1.cfg"), delim_whitespace=True, header=None)
    except IOError:
        return

    data.set_index(0, inplace=True)

    data.loc["BID_SIZE", 2] = pos
    data.loc["ASK_SIZE", 2] = pos
    data.loc["MIN_BID_DIME_SIZE", 2] = pos
    data.loc["MIN_ASK_DIME_SIZE", 2] = pos
    data.loc["MAX_BID_JOIN_SIZE", 2] = pos - 1
    data.loc["MAX_ASK_JOIN_SIZE", 2] = pos - 1

    token = config_folder.split("_")
    if token[-1] != "SQUAREOFF":
        data.loc["MAX_BID_OFFSET", 2] = secondary_stats.Px*max_offset_fraction
        data.loc["MAX_ASK_OFFSET", 2] = secondary_stats.Px*max_offset_fraction
        data.loc["MAX_BID_OFFSET_UPDATE", 2] = secondary_stats.Px*max_offset_fraction/4
        data.loc["MAX_ASK_OFFSET_UPDATE", 2] = secondary_stats.Px * max_offset_fraction / 4
    
    data.to_csv(os.path.join(config_folder, "Dimer1.cfg"), index=True, header=None, sep=" ")


def changeMQFile(config_folder, secondary_stats, tighten_fraction_, offset, pos):

    try:
        data = pd.read_csv(os.path.join(config_folder, "MultiQuoter1.cfg"), delim_whitespace=True, header=None)
    except IOError:
        return

    data.set_index(0, inplace=True)
    prev_tighten_percent_ = float(data.loc["BID_TIGHTEN_UPDATE_PERCENT", 2])

    data.loc["BID_SIZE", 2] = pos
    data.loc["ASK_SIZE", 2] = pos
    data.loc["STEP_OFFSET", 2] = max(int(secondary_stats.AvgSpd), 1)

    if prev_tighten_percent_ > 0:
        data.loc["BID_TIGHTEN_UPDATE_PERCENT", 2] = offset*tighten_fraction_
        data.loc["ASK_TIGHTEN_UPDATE_PERCENT", 2] = offset*tighten_fraction_

    data.to_csv(os.path.join(config_folder, "MultiQuoter1.cfg"), index=True, header=None, sep=" ")


def changeQuoterFile(config_folder, tighten_fraction_, offset, pos):

    try:
        data = pd.read_csv(os.path.join(config_folder, "Quoter1.cfg"), delim_whitespace=True, header=None)
    except IOError:
        return

    data.set_index(0, inplace=True)
    data.loc["BID_SIZE", 2] = pos
    data.loc["ASK_SIZE", 2] = pos
    prev_tighten_percent_ = float(data.loc["BID_TIGHTEN_UPDATE_PERCENT", 2])

    if prev_tighten_percent_ > 0:
        data.loc["BID_TIGHTEN_UPDATE_PERCENT", 2] = offset*tighten_fraction_
        data.loc["ASK_TIGHTEN_UPDATE_PERCENT", 2] = offset*tighten_fraction_

    data.to_csv(os.path.join(config_folder, "Quoter1.cfg"), index=True, header=None, sep=" ")


def changeModifyEyeFile(config_folder, pos):

    try:
        data = pd.read_csv(os.path.join(config_folder, "ModifyEye1.cfg"), delim_whitespace=True, header=None)
    except IOError:
        return

    data.set_index(0, inplace=True)
    data.loc["BID_SIZE", 2] = pos
    data.loc["ASK_SIZE", 2] = pos
    data.to_csv(os.path.join(config_folder, "ModifyEye.cfg"), index=True, header=None, sep=" ")


def changeElectronicEyeFile(eye_file, pos, offset, eye_multiplier_):

    try:
        data = pd.read_csv(eye_file, delim_whitespace=True, header=None)
    except IOError:
        return

    data.set_index(0, inplace=True)
    data.loc["SHOOT_SIZE", 2] = pos
    data.loc["BID_PERCENTAGE_OFFSET", 2] = offset*eye_multiplier_
    data.loc["ASK_PERCENTAGE_OFFSET", 2] = offset*eye_multiplier_    
    data.to_csv(eye_file, index=True, header=None, sep=" ")


def changeSingleFolderConfig(config_folder, max_pos_multiple, max_exp_multiple,
                             prod_stats, offset_spread_factor, min_offset, max_trade_value,
                             max_dimer_offset_fraction, tighten_fraction,
                             start_date_, end_date_, stats_folder_):

    primary_stats, secondary_stats, offset, pos, secondary_prod = changeMainConfig(config_folder,
                                                                                   prod_stats, offset_spread_factor,
                                                                                   min_offset, max_trade_value,
                                                                                   start_date_, end_date_,
                                                                                   stats_folder_)

    changeDimerFile(config_folder, secondary_stats, pos, max_dimer_offset_fraction)
    changeMQFile(config_folder, secondary_stats, tighten_fraction, offset, pos)
    changeQuoterFile(config_folder, tighten_fraction, offset, pos)
    changeModifyEyeFile(config_folder, pos)

    count = 1
    eye_file_ = os.path.join(config_folder, "ElectronicEye" + str(count) + ".cfg")
    eye_multiplier = 1
    while os.path.isfile(eye_file_):
        changeElectronicEyeFile(eye_file_, pos, offset, eye_multiplier)
        count += 1
        eye_multiplier = eye_multiplier*0.5
        eye_file_ = os.path.join(config_folder, "ElectronicEye" + str(count) + ".cfg")

    pos_string = secondary_prod + "_MAXLONGPOS = " + str(pos*max_pos_multiple) + \
                 "\n" + secondary_prod + "_MAXSHORTPOS = " + str(pos*max_pos_multiple) + \
                 "\n" + secondary_prod + "_MAXLONGEXPOSURE = " + str(pos*max_exp_multiple) + \
                 "\n" + secondary_prod + "_MAXSHORTEXPOSURE = " + str(pos*max_exp_multiple) + \
                 "\n"

    return pos_string


if __name__ == "__main__":

    max_pos_multiple_ = 2
    max_exp_multiple_ = 8
    offset_spread_factor_ = 1
    min_offset_ = 0.04
    max_trade_value_ = 250000
    max_dimer_offset_fraction_ = 0.008
    tighten_fraction_ = 0.5

    parser = argparse.ArgumentParser()
    parser.add_argument('strat_folder', help='Strat folder to update configs on')
    parser.add_argument('out_strat_folder', help='Folder in which updated strat folder will be created')
    parser.add_argument('stats_folder', help="Folder from which average stats are to be calculated")
    parser.add_argument('start_date', help='Date from which stats are to be calculated')
    parser.add_argument('end_date', help='Date till which stats are to be calculated')
    parser.add_argument('--max_pos_multiple', help='Multiple of size for Max position that is allowed')
    parser.add_argument('--max_exp_multiple', help='Multiple of size for Max exposure that is allowed')
    parser.add_argument('--min_offset', help='Min offset percentage')
    parser.add_argument('--max_trade_value', help='Max value of our order size')
    parser.add_argument('--max_dimer_offset_fraction', help='Multiple of price for Max offset Dimer')
    parser.add_argument('--tighten_fraction', help='Multiple of offset for tighten in quoter')

    args = parser.parse_args()

    if args.strat_folder:
        strat_folder = args.strat_folder
    else:
        sys.exit('Please provide input strat folder')

    if args.out_strat_folder:
        out_strat_folder = args.out_strat_folder
    else:
        sys.exit('Please provide folder in which updated strat folder will be created')

    if args.stats_folder:
        stats_folder = args.stats_folder
    else:
        sys.exit('Please provide input stats folder')

    if args.start_date:
        start_date = args.start_date
    else:
        sys.exit('Please provide start date')

    if args.end_date:
        end_date = args.end_date
    else:
        sys.exit('Please provide end date')

    if args.max_pos_multiple:
        max_pos_multiple_ = args.max_pos_multiple

    if args.max_exp_multiple:
        max_exp_multiple_ = args.max_exp_multiple

    if args.min_offset:
        min_offset_ = args.min_offset

    if args.max_trade_value:
        max_trade_value_ = args.max_trade_value

    if args.max_dimer_offset_fraction:
        max_dimer_offset_fraction_ = args.max_dimer_offset_fraction

    if args.tighten_fraction:
        tighten_fraction_ = args.tighten_fraction

    prod_stats_ = {}

    strat_name_ = os.path.basename(strat_folder)
    if not os.path.exists(out_strat_folder):
        os.makedirs(out_strat_folder)

    out_strat_folder = os.path.join(out_strat_folder, strat_name_ + "_" + start_date + "_" + end_date)

    try:
        shutil.rmtree(out_strat_folder)
    except OSError:
        pass

    copytree(strat_folder, out_strat_folder) 

    list_dir = [os.path.join(out_strat_folder,dI) for dI in os.listdir(out_strat_folder) if os.path.isdir(os.path.join(out_strat_folder,dI))]

    pos_list = [changeSingleFolderConfig(x, max_pos_multiple_, max_exp_multiple_,
                             prod_stats_, offset_spread_factor_, min_offset_, max_trade_value_,
                             max_dimer_offset_fraction_, tighten_fraction_,
                             start_date, end_date, stats_folder) for x in list_dir]

    pos_list = list(set(pos_list))
    posfile = os.path.join(out_strat_folder,"PositionLimits.csv")

    with open(posfile,"w") as f:
        f.write("GROSS_EXPOSURE_LIMIT = 100\nTOTAL_PORTFOLIO_STOPLOSS = 2000000\n\n")
        for s in pos_list:
            f.write(s)
