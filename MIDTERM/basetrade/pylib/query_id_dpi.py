#!/usr/bin/env python

"""

"""

import os
from pylib.setup_profile_plots import get_shortcode_for_query_id


from pylib.setup_profile_plots import get_t2t_stats_for_query
from pylib.profile_stats_utils import get_dpi_filepath

from walkforward.definitions.execs import paths


existing_dpi_factors = ['T2T', 'CXLREJ', 'QUEUEPOS']

class QueryIdDPI:
    """
    Create and store the query id DPI values
    """

    def __init__(self, query_id, tradingdate):
        self.tradingdate = tradingdate
        self.query_id = query_id

        self.queue_pos_stats = None
        self.cxl_rej_stats = None
        self.t2t_stats = None


        self.queue_pos_value = 0
        self.cxl_rej_value = 0
        self.t2t_value = 0

        self.queue_pos_weight = 1
        self.cxl_rej_weight = 10000
        self.t2t_weight = 1

        self.dpi_value = 0.0
        # the first element is always self.dpi_value (normalized dpi value)
        self.dpi_value_vec = [self.dpi_value]

        self.data_computed = False

        # check if data has already been computed
        filepath = get_dpi_filepath(query_id, tradingdate)
        if os.path.exists(filepath):
            # print('FILE', filepath)
            dpi_val_vec = list(map(float, open(filepath).readline().split()))
            if len(dpi_val_vec) > 0:
                print('DPI VALUE FOUND IN FILE ', dpi_val_vec, filepath)
                self.dpi_value_vec = dpi_val_vec
                self.data_computed = True

    def get_dpi_vec(self):
        """

        :return:
        """
        print('COMPUTE', self.dpi_value, self.t2t_value, self.t2t_weight, self.cxl_rej_value, self.cxl_rej_weight, self.queue_pos_value, self.queue_pos_weight)

        # multiply with weight and return
        dpi_vec = list(map(lambda x, y: x*y, self.dpi_value_vec, [1.0, self.queue_pos_weight, self.cxl_rej_weight, self.t2t_weight]))
        # cumulative DPI is simple sum
        dpi_vec[0] = sum(dpi_vec[1:])
        return dpi_vec

    def set_weights(self, qpos_wt, cxl_rej_wt, t2t_wt):
        """

        :param qpos_wt:
        :param cxl_rej_wt:
        :param t2t_wt:
        :return:
        """

        self.queue_pos_weight = qpos_wt
        self.cxl_rej_weight = cxl_rej_wt
        self.t2t_weight = t2t_wt

    def add_queue_pos_stats(self, queue_pos_stats):
        """
        # get the 90th %ile numbers
        # Filter -1 values too

        :param queue_pos_stats:
        :return:
        """
        if queue_pos_stats.empty:
            print('DATE', self.tradingdate, 'QUEUE_POS DATA FRAME EMPTY')
            return self

        self.queue_pos_stats = queue_pos_stats.copy()

        self.queue_pos_stats = queue_pos_stats[queue_pos_stats['y_axis'] > -1]
        self.queue_pos_value = queue_pos_stats['y_axis'].quantile(0.9)

        # print('DATE', self.tradingdate, self.queue_pos_value,  ' QUEUE', self.queue_pos_stats.shape[0])
        return self

    def add_cxl_rej_stats(self, cxl_rej_stats):
        """
        # get the 50th %ile numbers

        :param cxl_rej_stats:
        :return:
        """

        if cxl_rej_stats.empty:
            print('DATE', self.tradingdate, 'CXL_REJ DATAFRAME EMPTY')
            return self

        self.cxl_rej_stats = cxl_rej_stats.copy()
        self.cxl_rej_value = self.cxl_rej_stats['cxl_rejc'].astype(float).sum()/\
                             self.cxl_rej_stats['cxlseq'].astype(float).sum()

        # self.cxl_rej_stats['diff'] = cxl_rej_stats['y_axis'] - cxl_rej_stats['x_axis']
        # self.cxl_rej_value = self.cxl_rej_stats.shape[0]  # self.cxl_rej_stats['diff'].quantile(0.5)

        # print('DATE', self.tradingdate, self.cxl_rej_value, ' CXLREJ ', self.cxl_rej_stats.shape[0])

        return self

    def add_t2t_stats(self, t2t_stats):
        """

        :param t2t_stats:
        :return:
        """
        self.t2t_value = t2t_stats
        return self

    def compute_dpi(self):
        """

        :return:
        """
        self.dpi_value_vec = []
        self.dpi_value_vec.append(self.t2t_value)
        self.dpi_value_vec.append(self.cxl_rej_value)
        self.dpi_value_vec.append(self.queue_pos_value)

        # dpi value is always the first entry in the vector
        self.dpi_value_vec = [0] + self.dpi_value_vec

        # print('COMPUTE', self.dpi_value, self.t2t_value, self.t2t_weight, self.cxl_rej_value, self.cxl_rej_weight,
        # self.queue_pos_value, self.queue_pos_weight)
        return self.dpi_value

    def dump_dpi_to_eph(self):
        """

        :return:
        """
        if not self.dpi_value:
            # Raise error
            pass

        # later change it in a way that it doesn't rewrite same value to file, only if something changes
        filepath = get_dpi_filepath(self.query_id, self.tradingdate)
        f = open(filepath, "w")
        f.write(' '.join(list(map(str, self.dpi_value_vec))))
        f.close()

    def generate_dpi_value(self):
        """

        :return:
        """
        query_id = self.query_id
        tradingdate = self.tradingdate

        t2t_data = get_t2t_stats_for_query(query_id, tradingdate)

        if t2t_data:
            self.add_t2t_stats(t2t_data)
        else:
            # Add a failsafe later for cases where this value is not available
            self.add_t2t_stats(13)

        self.compute_dpi()
        self.dump_dpi_to_eph()
