#!/usr/bin/env python
import sys
import random
import numpy as np


class ArbSimulator:
    def __init__(self, filename, trades_file_name, days_to_expiry_filename, date):
        self.msecs = 0
        self.tom_bid = 0
        self.tom_ask = 0
        self.tom_mid = 0
        self.si_bid = 0
        self.si_ask = 0
        self.si_mid = 0
        self.decision = '-'
        self.position = 0
        self.max_position = 2
        self.last_traded_msecs = 0
        self.cool_off = 30000
        self.volume = 0
        self.pnl = 0
        self.filename = filename
        self.trades_file = open(trades_file_name, 'w')

    def MakeRandomDecision(self):
        random_val = random.random()
        if (random_val < 0.015):
            self.decision = 'B'
        elif (random_val > 0.985):
            self.decision = 'S'
        else:
            self.decision = '-'

    def ParseAndSimulate(self):
        lines = [line for line in open(self.filename, 'r').read().split('\n') if line != '']
        for line in lines:
            tokens = line.split(' ')
            self.msecs, self.si_bid, self.si_ask, self.si_mid, self.tom_bid, self.tom_ask, self.tom_mid = [
                float(p) for p in tokens[:]]
            self.tom_bid *= 1000
            self.tom_ask *= 1000
            self.tom_mid *= 1000
            self.decision = tokens[-1]
#            self.MakeRandomDecision()

            # Log some info about market and rates
            self.LogMktStatus()

            # Use the decision by strategy to compute PNL
            self.ComputePNL()

        # close open positions
        self.ClosePosition()

        # Print pnl, volume etc.
        self.PrintStats()

    def LogMktStatus(self):
        self.trades_file.write("Si:  [" + str(round(self.si_bid)) + " X " + str(round(self.si_ask)) + " ]\n")
        rateB, rateS, rate = self.strat.GetRates()
        self.trades_file.write("Tom: [" + str(round(self.tom_bid)) + " X " + str(round(self.tom_ask)) + " ] ")
        self.trades_file.write("[" + str(rateS) + " X " + str(rate) + " X " + str(rateB) +
                               " ] " + str(rateB - rate) + " : " + str(rate - rateS) + "\n")

    def LogBuyTrade(self):
        self.trades_file.write(str(int(self.msecs)) + " B Si @ " + str(self.si_ask) + " S TOM @ " +
                               str(self.tom_bid) + " " + str(self.pnl) + " " + str(self.volume) + " " + str(self.position) + "\n")

    def LogSellTrade(self):
        self.trades_file.write(str(int(self.msecs)) + " S Si @ " + str(self.si_bid) + " B TOM @ " +
                               str(self.tom_ask) + " " + str(self.pnl) + " " + str(self.volume) + " " + str(self.position) + "\n")

    def ComputePNL(self):
        # Buy Si ( Future ), Sell TOM ( Spot ) => Buy Spread
        if self.decision == 'B' and self.position < self.max_position and self.msecs - self.last_traded_msecs > self.cool_off:
            self.pnl += self.tom_bid
            self.pnl -= self.si_ask
            self.position += 1
            self.volume += 1
            self.LogBuyTrade()

        # Buy TOM ( Spot ), Sell Si ( Future ) => Sell Spread
        elif self.decision == 'S' and self.position > - self.max_position and self.msecs - self.last_traded_msecs > self.cool_off:
            self.pnl += self.si_bid
            self.pnl -= self.tom_ask
            self.position -= 1
            self.volume += 1
            self.LogSellTrade()
        # Do nothing
        else:
            pass

    def ClosePosition(self):
        # close open position
        # We have bought TOM and sold Si.
        if (self.position < 0):
            # Sell TOM, Buy Si
            self.pnl += self.tom_bid * self.position
            self.pnl -= self.si_ask * self.position
            self.volume += abs(self.position)
            self.position = 0
            self.LogBuyTrade()
        # We have sold tom, bought Si.
        elif (self.position > 0):
            # Buy TOM, Sell Si
            self.pnl += self.si_bid * self.position
            self.pnl -= self.tom_ask * self.position
            self.volume += self.position
            self.position = 0
            self.LogSellTrade()

    def PrintStats(self):
        print("PNL: " + str(self.pnl) + " VOL: " + str(self.volume))
        self.strat.DisplayStats()


if __name__ == "__main__":
    if (len(sys.argv) < 5):
        print("USAGE: Exec <data_filename> <expiry_filename> <trades_filename> <date>")
        exit(-1)
    data_file_name = sys.argv[1]
    trades_file_name = sys.argv[2]

    simulator = ArbSimulator(data_file_name, trades_file_name)
    simulator.ParseAndSimulate()
