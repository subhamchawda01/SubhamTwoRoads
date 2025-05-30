
import pandas as pd
import os
from utils import get_list_of_dates_given_start


class ProdStats:

    def __init__(self, prod_name, start_date, end_date, stats_folder):

        self.prod_name = prod_name
        self.stats_folder = stats_folder

        list_of_dates = get_list_of_dates_given_start(start_date, end_date)

        stats_list = [self.getstats(dt) for dt in list_of_dates]
        df = pd.DataFrame(stats_list, columns=["Px","Vol","Trades","L1Sz","TrdSz","AvgSpd"]).dropna()
        self.stats = df.mean()

    def getstats(self, dt):

        filepath = os.path.join(self.stats_folder, dt, self.prod_name + ".stats")
        if os.path.isfile(filepath):
            try:
                data = pd.read_csv(filepath, delim_whitespace=True, header=None)
                data.set_index(2, inplace=True)
                return [data.loc['avg_px_', 3], data.loc['total_volume_', 3], data.loc['total_trades_', 3],
                        data.loc['avg_l1_size_', 3], data.loc['avg_trd_sz_', 3], data.loc['avg_spread_', 3]]
            except Exception as e:
                return []
        else:
            return []

