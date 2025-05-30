#!/usr/bin/env python

"""
Major file for creating different plots etc

"""

import os
import pandas as pd
from collections import Counter

import dash
import dash_core_components as dcc
import dash_html_components as html
from flask_caching import Cache


from pylib.profile_stats_utils import get_one_day_shortcode_path
from pylib.profile_stats_utils import get_one_day_cxlrej_path
from pylib.profile_stats_utils import get_one_day_cxlrej_count_path
from pylib.profile_stats_utils import compute_one_days_stats
from pylib.profile_stats_utils import compute_one_day_cxlrej_stats
from pylib.profile_stats_utils import compute_one_day_cxlrej_count_stats

from pylib.profile_stats_utils import get_latency_stats_path

from pylib.definitions.ors_struct import ors_header_unique, ors_header_rep
from walkforward.wf_db_utils.db_handles import create_connection, change_db, connection
from walkforward.definitions.execs import paths


def get_queue_pos_data_for_shortcode(shortcode, tradingdate, axis_names):
    """

    :param shortcode:
    :param tradingdate:
    :param axis_names:
    :return:
    """
    filepath = get_one_day_shortcode_path(shortcode, tradingdate)

    # use pandas first
    # print(open(filepath).readline())
    col_names = ['px', 'saci', 'sz_rem', 'sz_exec', 'oid', 'q_send', 'q_conf', 'q_cxl_seq', 'q_cxld', 'q_exec',
                 'q_modify', 't_send', 't_conf', 't_cxl_seqd']

    if not os.path.exists(filepath):
        compute_one_days_stats(shortcode, tradingdate, filepath)

    # for c++ exec
    # SAOS  # PX #B/S #SACI #S_REM #S_EXEC        #ORDER_ID [ #Q_SEND #Q_CONF ( #Q_CXL_SEQ #P_CXL_SEQ ) ( #Q_CXLD #P_CXLD) ( #Q_MOD #P_MOD ) #Q_EXEC ] OE_BEF SE_BEF
    try:
        df = pd.read_csv(filepath, usecols=[1, 3, 4, 5, 6, 8, 9, 11, 15, 22, 19, 26, 28, 30], names=col_names)
        print(df.head(1))
    except IOError as e:
        print('ENCOUNTERED', e)
        df = pd.DataFrame(names=col_names)
    # for python script
    # df = pd.read_csv(filepath, sep='\s+', usecols=[3, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15, 17, 19, 21], names=col_names, engine='python')

    # depending on requirements, we can filterout appropriate line
    # the format is same as MarketDataStruct.to_string()

    num_col = len(df.columns)
    allowed_col = 4 * len(ors_header_rep) + len(ors_header_unique)

    # get only data till 4th repetition
    # df = df[df.columns[:allowed_col]]
    axis_values = pd.DataFrame(columns=['x_axis', 'y_axis'])
    for i in range(len(axis_names)):
        if axis_names[i] == 'original_queue_pos':
            axis_values[axis_values.columns[i]] = df['q_send'].copy()
        elif axis_names[i] == 'seqd_queue_pos_diff':
            axis_values[axis_values.columns[i]] = (df['q_conf'] - df['q_send']).copy()
        elif axis_names[i] == 'cancel_queue_pos':
            axis_values[axis_values.columns[i]] = df['q_cxl_seq'].copy()
    # print(axis_values)
    return axis_values


def plot_queue_pos_diff_histogram_for_shortcode(
        shortcode, tradingdate, axis_names=['seqd_queue_pos_diff']):
    """

    :param shortcode:
    :param tradingdate:
    :param axis_names:
    :return:
    """

    df = get_queue_pos_data_for_shortcode(shortcode, tradingdate, axis_names)
    count_words = Counter(df['x_axis'])
    return {
        'data': [{
            'x': list(count_words.keys()),
            'y': list(count_words.values())
        }],
    }


def plot_queue_pos_histogram_for_shortcode(
        shortcode, tradingdate, axis_names=['original_queue_pos']):
    """

    :param shortcode:
    :param tradingdate:
    :param axis_names:
    :return:
    """

    df = get_queue_pos_data_for_shortcode(shortcode, tradingdate, axis_names)
    count_words = Counter(df['x_axis'])
    return {
        'data': [{
            'x': list(count_words.keys()),
            'y': list(count_words.values())
        }],
    }

def plot_queue_pos_cancel_histogram_for_shortcode(
        shortcode, tradingdate, axis_names=['cancel_queue_pos']):
    """

    :param shortcode:
    :param tradingdate:
    :param axis_names:
    :return:
    """

    df = get_queue_pos_data_for_shortcode(shortcode, tradingdate, axis_names)
    count_words = Counter(df['x_axis'])
    return {
        'data': [{
            'x': list(count_words.keys()),
            'y': list(count_words.values())
        }],
    }

def plot_queue_pos_for_shortcode(shortcode, tradingdate, axis_names=[ 'original_queue_pos', 'seqd_queue_pos_diff'],
                                 lookback_days=20):
    """
    Plots the queue pos matrix for given shortcode for tradingdate
    In later stages we will add average over lookback_days

    :param shortcode:
    :param tradingdate:
    :param axis_names
    :param lookback_days
    :return:
    """

    # plot it here

    df = get_queue_pos_data_for_shortcode(shortcode, tradingdate, axis_names)
    return {
        'data': [{
            'x': df['x_axis'],
            'y': df['y_axis']
        }],
    }


def get_cxl_rej_stats_for_shortcode(shortcode, tradingdate):
    """

    :param shortcode:
    :param tradingdate:
    :return:
    """
    filepath = get_one_day_cxlrej_path(shortcode, tradingdate)
    if not os.path.exists(filepath):
        compute_one_day_cxlrej_stats(shortcode, tradingdate, filepath)

    df = pd.DataFrame()
    col_names = ['CxldSeq-TS', 'Exec-TS', 'SACI']
    if os.stat(filepath).st_size > 0:
        df = pd.read_csv(filepath, sep='\s+')
        df.columns = ['x_axis', 'y_axis', 'SACI']
    return df


def get_cxl_rej_count_stats_for_shortcode(shortcode, tradingdate):
    """

    :param shortcode:
    :param tradingdate:
    :return:
    """

    filepath = get_one_day_cxlrej_count_path(shortcode, tradingdate)
    if not os.path.exists(filepath):
        compute_one_day_cxlrej_count_stats(shortcode, tradingdate, filepath)

    df = pd.DataFrame()
    if os.stat(filepath).st_size > 0:
        df = pd.read_csv(filepath, sep='\s+')
        df.columns = ['cxlseq', 'cxld', 'cxl_rejc', 'saci']

    return df

def get_t2t_stats_for_query(query_id, tradingdate):
    """

    :param query_id:
    :param tradingdate:
    :return:
    """

    filepath = get_latency_stats_path(tradingdate, query_id)

    if not os.path.exists(filepath):
        return None

    f = open(filepath)
    lines = f.readlines()

    try:
        t2t_value = lines[1].split(":")[-1].split()[3]
    except BaseException:
        return None

    return float(t2t_value)


class Mapper(object):
    map_store = {}

    def add_mapping(self, class1, class2, val1, val2):
        '''
        The idea is to create a Mapper which can handle chained
        mappings like class1->class2->class3
        This will be beneficial when we will add Exchange level
        support for DPI.

        One way to do is to replicate the mappings across all keys - Redundant Space. Computation on addition.
        The other way is to proceed with iteration across all keys - Redundant Computation.

        We'll go through the first way because once we compute all such mappings, we
        can simply dump them to disk and read/update when required.
        '''

        self.create_single_bimap(class1, class2, val1, val2)

        '''
            We have made bidirectional mappings
            Now we have to make mappings with all other classes too.
        '''

        for class3 in self.map_store[class1]:
            if class3 == class1:
                continue
            for key in self.map_store[class1][class3]:
                if key == val1:
                    continue
                for value in self.map_store[class1][class3][key]:
                    self.create_single_bimap(class3, class2, val1, val2)

    def create_single_bimap(self, class1, class2, val1, val2):
        if class1 not in self.map_store:
            self.map_store[class1] = {}

        if class2 not in self.map_store:
            self.map_store[class2] = {}

        if class1 not in self.map_store[class2]:
            self.map_store[class1][class2] = {}

        if class1 not in self.map_store[class2]:
            self.map_store[class2][class1] = {}

        if val1 not in self.map_store[class1][class2]:
            self.map_store[class1][class2][val1] = []

        if val2 not in self.map_store[class2][class1]:
            self.map_store[class2][class1][val2] = []

        if val2 not in self.map_store[class1][class2][val1]:
            self.map_store[class1][class2][val1] += [val2]

        if val1 not in self.map_store[class2][class1][val2]:
            self.map_store[class2][class1][val2] += [val1]

    def get_mappings(self, key_class, val, value_class):
        """

        :param key_class:
        :param val:
        :param value_class:
        :return:
        """
        paired_map = self.get_map_pair(key_class, value_class)
        if paired_map is None or val not in paired_map.keys():
            return None

        return paired_map[val]

    def get_map_pair(self, key_class, value_class):
        """

        :param key_class:
        :param value_class:
        :return:
        """
        if key_class not in self.map_store:
            return None
        if value_class not in self.map_store[key_class]:
            return None

        return self.map_store[key_class][value_class]


def get_shortcode_for_query_id(query_id, tradingdate=0):
    """

    :param query_id:
    :param tradingdate:
    :return:
    """
    mapping = Mapper().get_mappings("query", query_id, "shortcode")
    if mapping:
        print("Mapping found: ", mapping, "q-s")
        return mapping[0]

    create_connection("52.87.81.158", "dvcreader", "f33du5rB", "results")
    select_query = (
        "SELECT * from PickstratConfig where start_queryid <= " +
        str(query_id) +
        " and end_queryid >= " +
        str(query_id))
    cursor = connection().cursor()
    cursor.execute(select_query)
    rows = cursor.fetchall()
    min_diff = 1000000
    shortcode = None
    for row in rows:
        diff = row[3] - row[2]
        if diff < min_diff:
            min_diff = diff
            shortcode = row[1].split('.')[0]

    Mapper().add_mapping("query", "shortcode", query_id, shortcode)
    return shortcode


def get_query_id_list_for_shortcode(shortcode):
    """

    :param query_id:
    :param tradingdate:
    :return:
    """
    mapping = Mapper().get_mappings("shortcode", shortcode, "query")
    if mapping:
        print("Mapping found: ", mapping, "s-q")
        return mapping

    create_connection("52.87.81.158", "dvcreader", "f33du5rB", "results")
    select_query = (
        "SELECT * from PickstratConfig where config_name like '" +
        shortcode +
        "%';")
    print(select_query)
    cursor = connection().cursor()
    cursor.execute(select_query)
    rows = cursor.fetchall()
    query_id_list = []
    for row in rows:
        id_list = list(range(row[2], row[3] + 1))
        query_id_list += [str(x) for x in id_list]

    for query_id in query_id_list:
        Mapper().add_mapping("shortcode", "query", shortcode, query_id)

    return query_id_list
