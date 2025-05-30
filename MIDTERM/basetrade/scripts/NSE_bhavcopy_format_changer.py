import sys
from datetime import datetime


def f(s):
    return "{0:010.2f}".format(float(s))


if len(sys.argv) < 2:
    print("USAGE: <exec> <website_bhavcopy_file>")
    exit()

file_name = sys.argv[1]
# fo14JAN2015bhav.csv

if not file_name[0:2] == "fo":
    exit()

new_file_name = datetime.strptime(file_name[2:11].lower(), "%d%b%Y").strftime("%d%m") + "fo_0000.md"

new_file = open(new_file_name, "w")

lines = open(file_name).read().split('\n')[1:]

for line in lines:
    # Non empty lines
    if len(line) > 5:
        tokens = line.split(',')
#         print tokens
        inst, symbol, date, strike, opt_type, open, high, low, close, settle, contracts, val, oi, ch_oi, _, _ = tokens
        str = "NORMAL ," + inst + " ," + symbol
        for i in range(0, 11 - len(symbol)):
            str += " "
        str += ","
        date = datetime.strptime(date, "%d-%b-%Y").strftime("%d %b %Y")
        date = date.upper()
        str += date + ","

        if (opt_type == "XX"):
            str += "           ,   ,"
        else:
            str += strike
            for i in range(0, 11 - len(strike)):
                str += " "
            str += "," + opt_type + " ,"

        str += "00000000000.00," + f(open) + "," + f(high) + "," + f(low) + "," + \
            f(settle) + "," + "0000000000," + f(val) + "," + oi + "," + ch_oi
        str += "\n"
        new_file.write(str)


new_file.close()
