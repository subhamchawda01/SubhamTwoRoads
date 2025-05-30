import fileinput
import subprocess
shc_pnl = {}
shc_pos = {}
shc_vol = {}
shortcodes = {}
global_pnl = 0.0

for line in fileinput.input():
    words = line.split()
    shc = (words[3].split(',')[1])
    shc_pnl[shc] = float(words[3].split(',')[2])
    shc_pos[shc] = int(words[3].split(',')[5])
    shc_vol[shc] = int(words[3].split(',')[6])
    exch_cmd = "/home/pengine/prod/live_execs/get_contract_specs " + shc + " `date +%Y%m%d` EXCHANGE | awk '{print $2}'"
    exch = subprocess.Popen(exch_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            stdin=subprocess.PIPE, shell=True).communicate()[0].strip()
    shortcodes.setdefault(exch, []).append(shc)

for exch in shortcodes:
    print("")
    exch_pnl = 0.0
    exch_vol = 0
    for shc in shortcodes[exch]:
        exch_pnl += shc_pnl[shc]
        exch_vol += shc_vol[shc]
        print("  ", shc, shc_pnl[shc], shc_pos[shc], shc_vol[shc])
    print("EXCHANGE: ", exch, exch_pnl, exch_vol)
    global_pnl += exch_pnl

print("ALL EXCHANGES: ", global_pnl)
