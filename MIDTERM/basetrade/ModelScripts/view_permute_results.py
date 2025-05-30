#!/usr/bin/env python

import sys
import os
import argparse
import numpy

def get_permuted_params( filename ):
    f = open( filename, "r" )
    output = []
    for line in f:
        temp = line.split( " " )
        if len ( temp ) > 3 and not line.startswith('#'):
            output.append( temp[1] )
    return output

def get_permuted_params_value( filename, permuted_params ):
    params = [line.rstrip('\n') for line in open( filename )]
    current = 0
    output = []
    for i in params:
        temp = i.split( " " )
        if len(temp) < 2 or i.startswith('#'):
            continue
            #print(temp)
        elif len(permuted_params) <= current:
            break
        elif permuted_params[current] == temp[1]:
            output.append(temp[2])
            current = current + 1
    return output

def get_pnl_stats( pnl_content, num_sharpe_days, least_count ):
    positive_days = sum(1 for x in pnl_content if x>0)
    output = [str(positive_days)]
    pnl_content_np = numpy.array(pnl_content)
    for day in range(0,num_sharpe_days):
        if (day+1)*least_count >= len(pnl_content_np):
            output.append('na')
        else:
            if day == 0:
                temp = pnl_content_np[-1*(day+1)*least_count:]
            else:
                temp = pnl_content_np[-1*(day+1)*least_count:-1*day*least_count]
            output.append( str(format(numpy.mean(temp)/numpy.std(temp),'.2f')) )
    return output

parser = argparse.ArgumentParser()
parser.add_argument('-p', dest='Param', help="permute paramfile", type=str, required=True)
parser.add_argument('-d', dest='directory', help="permute workdir", type=str, required=True)
parser.add_argument('-sd', dest='start_date', help="start_date", type=str, default='20120101')
parser.add_argument('-ed', dest='end_date', help="end_date", type=str, default='20200101')
parser.add_argument('-skipdays', dest='skip_days', help="skip_days", type=str, default='IF')
parser.add_argument('-selectdays', dest='select_days', help="select_days", type=str, default='IF')
parser.add_argument('-sort', dest='sort_algo', help="sort_algo", type=str, default='kCNAPnlMaxLoss')
args = parser.parse_args()


permuted_params = get_permuted_params( args.Param )

summarise_results_cmd = "~/basetrade_install/bin/summarize_strategy_results . " + args.directory + "/strats_dir/ " + args.directory + "/local_results_base_dir/ "\
                        + args.start_date + " " + args.end_date + " " + args.skip_days + " " + args.sort_algo + " 0 " + args.select_days + " 0 "\
                        + " > " + args.directory + "/summarise"

print(summarise_results_cmd)
os.system( summarise_results_cmd )

contents = [line.rstrip('\n') for line in open(args.directory+"/summarise")]

num_sharpe_days = 5
least_count = 20
output = []
output.append( ["sname"] + permuted_params + [ "PnL", "Sharpe", "MaxLoss", "Volume", "PositiveDays", "0-20", "20-40", "40-60", "60-80", "80-100" ] )
for i in contents:
    temp = i.split( " " )
    if temp[0] == "STRATEGYFILEBASE":
        stratbase = temp[1][temp[1].rfind('_')+1:]
        stratfile = args.directory + "/strats_dir/" + temp[1]
        stratcontents = [line.strip('\n') for line in open( stratfile )]
        param_file = stratcontents[0].split(" ")[4]
        params = get_permuted_params_value( param_file, permuted_params )
        strat_overall_output = []
    elif temp[0] == "STATISTICS":
        param_output = [ temp[1], temp[4], temp[22], temp[3] ]
        stats_output = get_pnl_stats( strat_overall_output, num_sharpe_days, least_count )
        output.append( [stratbase] + params + param_output + stats_output )
    elif len(temp) >= 2:
        strat_overall_output.append( int(temp[1]) ) 

for line in output:
    for element in line:
        sys.stdout.write(element)
        sys.stdout.write('\t')
    sys.stdout.write('\n')


