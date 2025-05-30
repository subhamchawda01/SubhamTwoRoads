#!/usr/bin/env python

"""

"""

import os

import pandas as pd


from pylib.setup_profile_plots import get_queue_pos_data_for_shortcode
from pylib.setup_profile_plots import get_cxl_rej_stats_for_shortcode
from pylib.setup_profile_plots import get_cxl_rej_count_stats_for_shortcode
from pylib.setup_profile_plots import get_query_id_list_for_shortcode

from pylib.profile_stats_utils import get_latency_stats_path
from pylib.profile_stats_utils import get_dpi_filepath
from pylib.profile_stats_utils import get_weight_path

from pylib.query_id_dpi import existing_dpi_factors
from pylib.query_id_dpi import QueryIdDPI


class ShortcodeDPI:
    def __init__(self, shortcode, tradingdate):
        self.shortcode = shortcode
        self.tradingdate = tradingdate

        # used to differentiate between cases when we want to compute data for all queries for the shortcode vs given query list

        self.compute_for_all_queries = False
        self.query_id_list = []
        self.saci_list = []
        self.query_id_dpi_list = []

        self.query_id_to_idx = {}

        self.queue_pos_data = pd.DataFrame()

        self.cxl_rej_data = pd.DataFrame()

        # for shortcode if data already exists in file
        self.data_exists = False
        # for shortcode if the data has been computed for all shortcode for query
        self.data_computed = False

        # if we are using all queries for shortcodes
        self.all_queries = False

        # shortcode DPI value
        self.dpi_value = 0
        # maintain the individual scores
        self.dpi_value_vec = []

        self.queue_pos_weight = 1
        self.cxl_rej_weight = 10000
        self.t2t_weight = 1

        self.read_weights_from_file()

    def read_weights_from_file(self):
        """

        :return:
        """
        # try reading the weights from file
        wt_path = get_weight_path(self.shortcode)
        print('FILENAME')
        if os.path.exists(wt_path):
            lines = open(wt_path).readlines()
            for line in lines:
                line = line.split()
                if len(line) > len(existing_dpi_factors) and self.shortcode == line[0]:
                    print('LOADING WEIGHTS', ' '.join(line))
                    self.queue_pos_weight = float(line[1])
                    self.cxl_rej_weight = float(line[2])
                    self.t2t_weight = float(line[3])
                    break

    def set_compute_data_for_all_query_id(self, flag):
        """
        flag setting and populating the query-id lists
        :param flag:
        :return:
        """
        self.compute_for_all_queries = flag
        t_query_id_list = get_query_id_list_for_shortcode(self.shortcode)

        for query_id in t_query_id_list:
            self.add_query_id(query_id)
            # break

        filepath = get_dpi_filepath(self.shortcode, self.tradingdate)
        if os.path.exists(filepath):
            dpi_val_vec = list(map(float, open(filepath).readline().split()))
            if len(dpi_val_vec) > 0:
                self.dpi_value = dpi_val_vec[0]
                self.dpi_value_vec = dpi_val_vec
                self.data_exists = True

        self.all_queries = True
        self.data_computed = True

        if not self.data_exists:
            # check for all data-computed for all queries and set it to true
            for query_dpi in self.query_id_dpi_list:
                if not query_dpi.data_computed:
                    self.data_computed = False

    def add_query_id(self, query_id):
        """
        Add query id to list we want to process
        :param query_id:
        :return:
        """

        # first line here is SACI
        filepath = get_latency_stats_path(self.tradingdate, query_id)
        this_saci = -1
        if os.path.exists(filepath):
            this_saci = open(filepath).readline()

        self.saci_list.append(int(this_saci))
        self.query_id_list.append(query_id)

        self.query_id_to_idx[query_id] = len(self.query_id_list) -1

        # print('CALLING, ', query_id, self.tradingdate, self.shortcode)
        query_dpi = QueryIdDPI(query_id, self.tradingdate)

        # set the weight of factor for the query
        query_dpi.set_weights(self.queue_pos_weight, self.cxl_rej_weight, self.t2t_weight)

        self.query_id_dpi_list.append(query_dpi)

    def compute_dpi(self):
        """

        :return:
        """
        if self.all_queries:
            count_vec = [0]*(len(existing_dpi_factors) + 1)
            self.dpi_value_vec = [0]*(len(existing_dpi_factors) + 1)
            for idx in range(len(self.query_id_dpi_list)):
                if self.query_id_dpi_list[idx].data_computed:
                    # compute the sum of individual DPI
                    self.dpi_value_vec = list(map(lambda x, y: (x+y), self.query_id_dpi_list[idx].dpi_value_vec, self.dpi_value_vec))
                    # keep track of instances where one of the constituent is zero
                    count_vec = list(map(lambda x, y: y+1 if x > 0 else y, self.query_id_dpi_list[idx].dpi_value_vec, count_vec))

            # normalize the dpi_value_Vec with individual occurances
            for idx in range(len(count_vec)):
                if count_vec[idx] > 0:
                    self.dpi_value_vec[idx] /= count_vec[idx]

            # update the first value (DPI)
            self.dpi_value = self.dpi_value_vec[0]

    def compute_data(self):
        """

        :return:
        """
        # this we will have to compute irrespective of number of queries
        if self.data_computed:
            self.compute_dpi()
            return

        if self.queue_pos_data.empty:
            self.queue_pos_data = get_queue_pos_data_for_shortcode(self.shortcode, self.tradingdate,
                                                                   axis_names=['original_queue_pos', 'seqd_queue_pos_diff'])
        if self.cxl_rej_data.empty:
            # self.cxl_rej_data = get_cxl_rej_stats_for_shortcode(self.shortcode, self.tradingdate)
            self.cxl_rej_data = get_cxl_rej_count_stats_for_shortcode(self.shortcode, self.tradingdate)

        for idx in range(len(self.query_id_list)):
            if not self.query_id_dpi_list[idx].data_computed:
                # compute the data for individual queries

                self.query_id_dpi_list[idx].add_queue_pos_stats(self.queue_pos_data)
                self.query_id_dpi_list[idx].add_cxl_rej_stats(self.cxl_rej_data)

                # generate the remaining data and dump it
                self.query_id_dpi_list[idx].generate_dpi_value()
                self.query_id_dpi_list[idx].data_computed = True

            self.query_id_dpi_list[idx].compute_dpi()

        # as of now data would have gotten computed for all queries
        self.data_computed = self.all_queries
        self.compute_dpi()

    def get_dpi_value(self, vector=False):
        """
        return the dpi value for a shortcode
        :return:
        """
        self.compute_dpi()
        dpi_vec = list(map(lambda x, y: x * y, self.dpi_value_vec, [1.0, self.queue_pos_weight, self.cxl_rej_weight, self.t2t_weight]))
        dpi_vec[0] = sum(dpi_vec[1:])

        return dpi_vec if vector else dpi_vec[0]

    def get_dpi_value_for_query_id(self, query_id, vector=False):
        """
        DPI value for given query_id
        :param query_id:
        :return:
        """
        print('GET_DPI FOR QUERY_ID', query_id)
        index = -1
        if query_id in self.query_id_to_idx.keys():
            index = self.query_id_to_idx[query_id]

        return self.query_id_dpi_list[index].get_dpi_vec() if vector else self.query_id_dpi_list[index].get_dpi_vec()[0]