#!/usr/bin/env python

"""

Read the filesource for given product for given feed(it's of mostly)

"""

import subprocess

from walkforward.definitions.execs import execs
from walkforward.definitions.execs import execs


def get_market_data_for_shortcode(shortcode, tradingdate):
    """

    :param shortcode:
    :param tradingdate:
    :return:
    """

    get_path_cmd = [execs().get_path, shortcode, tradingdate, 'OF']

    get_path_out = subprocess.check_output(get_path_cmd)

    filename = get_path_out[-1]

    mds_log_reader_cmd = [execs().mds_log_reader, ]