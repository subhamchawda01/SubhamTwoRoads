#!/usr/bin/env python

"""
This plots the profiling data

"""

import os
import subprocess

from walkforward.definitions.execs import paths, execs


def get_latency_stats_path(tradingdate, query_id):
    """

    :param tradingdate:
    :param query_id:
    :return:
    """
    filepath = os.path.join(paths().dpi_logs, tradingdate[:4], tradingdate[4:6], tradingdate[6:], 'latency_stats.' +
                            query_id)
    return filepath



def get_dpi_filepath(query_id, tradingdate):
    """
    move it to db at later stage, the database is already created
    :param query_id
    :param tradingdate
    :return:
    """
    filepath = os.path.join(paths().shared_ephemeral_fbpa,
                            'profile_stats',
                            'dpi.' + query_id + '.' + str(tradingdate))
    return filepath


def get_weight_path(shortcode):
    """

    :param query_id:
    :return:
    """
    filepath  = os.path.join(paths().shared_ephemeral_fbpa, 'profile_stats', 'factor_weights' )
    return filepath

def get_one_day_shortcode_path(shortcode, tradingdate):
    """

    :param shortcode:
    :param tradingdate:
    :return:
    """
    ephemeral = os.path.join(paths().shared_ephemeral_fbpa, 'profile_stats')
    filepath = os.path.join(ephemeral, 'queue_pos.' + shortcode +'.' + str(tradingdate))

    return filepath


def get_one_day_cxlrej_path(shortcode, tradingdate):
    """

    :param shortcode:
    :param tradingdate:
    :return:
    """
    filepath = os.path.join(
        paths().shared_ephemeral_fbpa, 'profile_stats', 'cxl_rej.' + shortcode + '.' + str(tradingdate))

    return filepath

def get_one_day_cxlrej_count_path(shortcode, tradingdate):
    """

    :param shortcode:
    :param tradingdate:
    :return:
    """
    filepath = os.path.join(
        paths().shared_ephemeral_fbpa, 'profile_stats', 'cxl_rej.count.' + shortcode + '.' + str(tradingdate))

    return filepath


def compute_one_days_stats(shortcode, tradingdate, filename):
    """
    # filter out later for exchanges where ordercount is always 1, use size
    # there
    :param shortcode:
    :param tradingdate:
    :param filename:
    :return:
    """

    cmd = [execs().print_queue_stats, shortcode, str(tradingdate) ]
    # cmd = [execs().get_queue_position, '-s', shortcode, '-d', str(tradingdate), '-osz', '1', '-b', '1']
    print('RUNNING', ' '.join(cmd))

    try:
        out = subprocess.check_output(cmd).decode('utf-8')
        filehandle = open(filename, 'w')
        filehandle.write(out)
        filehandle.close()
    except IOError as e:
        print('ERROR: ', ' '.join(cmd), e)


def compute_one_day_cxlrej_stats(shortcode, tradingdate, filename):
    """
    ignoring 50th percentile here
    :param shortcode:
    :param tradingdate:
    :param filename:
    :return:
    """
    cmd = [execs().get_cxl_rej_stats2, '-shc', shortcode, '-date', str(tradingdate), '-o', filename]
    try:
        out = subprocess.check_output(cmd).decode('utf-8')
    except IOError as e:
        print('CXL_REJ ERROR', ' '.join(cmd), e)


def compute_one_day_cxlrej_count_stats(shortcode, tradingdate, filename):
    """

    :param shortcode:
    :param tradingdate:
    :param filename:
    :return:
    """

    cmd = [execs().get_cxl_rej_stats2, '-shc', shortcode, '-date', str(tradingdate), '-o', filename, '-c', '1']
    try:
        out = subprocess.check_output(cmd).decode('utf-8')
    except IOError as e:
        print('ERROR: ', ' '.join(cmd), e)
