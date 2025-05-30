import sys
from math import floor


def GetBondPrice(price, term):
    if price > 0:
        y = 100 - price
        j8 = y / 200.0
        j9 = 1 / (1 + j8)
        j10 = 1000 * (3 * (1 - (j9**(term * 2))) / j8 + 100 * (j9**(term * 2)))
        return j10
    else:
        return 0


if len(sys.argv) < 3:
    print("USAGE: <exec> xt_price yt_price")
    exit(0)

xt_price = float(sys.argv[1])
yt_price = float(sys.argv[2])

xt_price_diff = GetBondPrice(xt_price, 10) - GetBondPrice(xt_price - 0.01, 10)
yt_price_diff = GetBondPrice(yt_price, 3) - GetBondPrice(yt_price - 0.01, 3)

xt_factor = 10
yt_factor = int(floor(xt_price_diff / yt_price_diff * xt_factor))

print("XTYT " + str(xt_factor) + str(yt_factor))
