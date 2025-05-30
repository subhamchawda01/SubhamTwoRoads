#!/usr/bin/python
import datetime
import os
import sys
import fcntl

SCRIPTS_DIR = os.environ['HOME'] + '/basetrade/scripts'

USAGE = "<start_date> <end_date> <periodicity(in weeks)> [<recompute>] [<input_file_portfolio_dep_indep> <output_dir>]"

if(len(sys.argv) < 4):
    print(USAGE + "\n")
    sys.exit(0)

start_date = sys.argv[1]
end_date = sys.argv[2]
periodicity = max(1, int(sys.argv[3]))

recompute = "0"
if len(sys.argv) > 4:
    recompute = sys.argv[4]

if len(sys.argv) > 5 and len(sys.argv) < 7:
    print(USAGE)
    print("both [<input_file_portfolio_dep_indep> <output_dir>] should be present")
    sys.exit(0)

portfolio_dir = "/spare/local/tradeinfo/SupervisedPortInfo"
port_inputs_file = portfolio_dir + '/portfolio_inputs'
if len(sys.argv) > 6:
    port_inputs_file = sys.argv[5]
    portfolio_dir = sys.argv[6]

trend_seconds = 120

end_date = datetime.datetime.strptime(end_date, '%Y%m%d')
last_friday_date = str((end_date - datetime.timedelta((3 + end_date.weekday()) % 7)).strftime('%Y%m%d'))

REG_COEFF_SCRIPT = SCRIPTS_DIR + '/generate_regport_coeffs.pl'

instructions_vec = []

while (int(last_friday_date) > int(start_date)):
    outfile = portfolio_dir + '/reg_weights_' + last_friday_date
    exec_cmd = REG_COEFF_SCRIPT + ' ' + port_inputs_file + ' ' + outfile + \
        ' ' + str(trend_seconds) + ' ' + last_friday_date + ' ' + recompute
    instructions_vec.append(exec_cmd)

    last_friday_date = str((datetime.datetime.strptime(last_friday_date, '%Y%m%d') -
                            datetime.timedelta(7 * periodicity)).strftime('%Y%m%d'))

for exec_cmd in instructions_vec:
    print(exec_cmd)
    os.system(exec_cmd)
