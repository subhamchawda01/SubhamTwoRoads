#!/usr/bin/env python
import sys
import random
import numpy as np


class Strategy:
    def __init__(self, days_to_expiry_filename, date):
        self.rate = 0  # R^(n-1)
        self.date = date
        self.rate_list = []
        self.date_to_expiry_map = {}
        self.projected_tom_bid = 0
        self.projected_tom_ask = 0
        self.rateB = 0
        self.rateS = 0
        self.rateB_diff = []
        self.rateS_diff = []
        self.thresh = 0.0002
        self.cool_off = 30
        self.max_pos = 20
        self.last_trade_msecs = 0
        self.position = 0
        self.ParseDaysToExpiry(days_to_expiry_filename)
        self.n = self.date_to_expiry_map[self.date]
        if self.n <= 1:
            self.n = 2

    def GetRates(self):
        return (self.rateB, self.rateS, self.rate)

    def ParseDaysToExpiry(self, filename):
        lines = [line for line in open(filename, 'r').read().split('\n') if line != '']
        for i in range(0, len(lines) - 1, 2):
            date = int(lines[i])
            expiry = int(lines[i + 1])
            self.date_to_expiry_map[date] = expiry

    def MakeDecision(self, si_bid, si_ask, si_mid, tom_bid, tom_ask, tom_mid, msecs):
        if tom_mid > 0:
            self.rate = si_mid / tom_mid - 1
            self.rate_list.append(self.rate)

            # Only take decisions after 600 seconds
            if(len(self.rate_list) > 600):

                # always keep running 600 info in rate_list
                self.rate_list.pop(0)
                self.rate = np.mean(self.rate_list)

                # project tom's prices in si space
                self.projected_tom_bid = (1 + self.rate) * tom_bid
                self.projected_tom_ask = (1 + self.rate) * tom_ask

                # Buy Rate and Sell Rate to aggress for arbitrage
                self.rateB = si_ask / tom_bid - 1
                self.rateS = si_bid / tom_ask - 1

                # Just for info, not required in strat
                self.rateB_diff.append(self.rateB - self.rate)
                self.rateS_diff.append(self.rate - self.rateS)

                # Don't take a decision within cooloff seconds of last trade
                if (msecs - self.last_trade_msecs < self.cool_off):
                    return '-'

                # rateB > rateS by design
                # Condition for BUY spread ( buy Si, sell TOM )
                if (self.rateB + self.thresh < self.rate):
                    # sell tom and buy si
                    if (self.position >= self.max_pos):
                        return '-'
                    self.position += 1
                    self.last_trade_msecs = msecs
                    return 'B'
                # Condition for SELL spread ( sell TOM, buy Si )
                elif (self.rateS > self.rate + self.thresh):
                    # buy tom and sell si
                    if (self.position <= -self.max_pos):
                        return '-'
                    self.position -= 1
                    self.last_trade_msecs = msecs
                    return 'S'
                else:
                    return '-'
        return '-'

    def DisplayStats(self):
        print("MeanRate(n-1): " + str(np.mean(self.rate_list)) + " ,MedianRate(n-1): " +
              str(np.median(self.rate_list)) + " ,StdDevRate(n-1): " + str(np.std(self.rate_list)))
        print("DATE: " + str(self.date) + " : R1( OneDayRate ) : " + str(np.mean(self.rate_list) /
                                                                         (self.n - 1)) + " R( Yearly ) : " + str((np.mean(self.rate_list) / (self.n - 1)) * 365))
        print("DiffRateB Mean: " + str(np.mean(self.rateB_diff)) + " , Max: " +
              str(np.max(self.rateB_diff)) + " , Min: " + str(np.min(self.rateB_diff)))
        print("DiffRateS Mean: " + str(np.mean(self.rateS_diff)) + " , Max: " +
              str(np.max(self.rateS_diff)) + " , Min: " + str(np.min(self.rateS_diff)))


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
        self.volume = 0
        self.pnl = 0
        self.filename = filename
        self.trades_file = open(trades_file_name, 'w')
        self.strat = Strategy(days_to_expiry_filename, date)

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
            # Either the decision comes from last column of a file or from the MakeDecision call of strategy
#             self.decision = tokens[-1]
            # Random strat, buys 15% of time and sell 15% of time, rest does nothing
#             self.MakeRandomDecision()
            # Decision of B,S, or - comes from the strategy
            self.decision = self.strat.MakeDecision(
                self.si_bid, self.si_ask, self.si_mid, self.tom_bid, self.tom_ask, self.tom_mid, self.msecs)

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
        if self.decision == 'B':
            self.pnl += self.tom_bid
            self.pnl -= self.si_ask
            self.position += 1
            self.volume += 1
            self.LogBuyTrade()

        # Buy TOM ( Spot ), Sell Si ( Future ) => Sell Spread
        elif self.decision == 'S':
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
    expiry_file_name = sys.argv[2]
    trades_file_name = sys.argv[3]
    date = int(sys.argv[4])

    simulator = ArbSimulator(data_file_name, trades_file_name, expiry_file_name, date)
    simulator.ParseAndSimulate()
