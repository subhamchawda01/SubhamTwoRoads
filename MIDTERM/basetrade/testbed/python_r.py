import subprocess
import sys
import os
import argparse

sys.path.append(os.path.expanduser('~/basetrade/'))
from walkforward.wf_db_utils.db_handles import connection
from walkforward.definitions import execs

parser = argparse.ArgumentParser()
parser.add_argument('-i1', dest='regdata_filename', help="regdata_filenaame", type=str, required=True)
parser.add_argument('-i2', dest='reg_output_file',  help="output filename", type=str, required=True)
parser.add_argument('-i3', dest='regress_exec_params',  help="regression params", type=str, required=True)

args = parser.parse_args()

siglr_cmd = [execs.execs().siglr, args.regdata_filename, args.reg_output_file, args.regress_exec_params]
process = subprocess.Popen(' '.join(siglr_cmd), shell=True,
                           stderr=subprocess.PIPE,
                           stdout=subprocess.PIPE)
out, err = process.communicate()
errcode = process.returncode

if len(err) > 0:
    print("Error reported in SIGLR call" + err)
    print("KP1")
    print(errcode)
    print("KP2")
    # if errcode != 0:
    #    raise ValueError("Error reported in SIGLR call" + errcode)
print(err)
print(out)
