#!/usr/bin/env python

"""

For a given shortcode, it finds n strats which have results for some days in given day range

"""

import os
import subprocess

from walkforward.definitions.execs import execs
from Tests.global_tests_utils.get_strats_for_shortcode import get_strats_for_shortcode


def get_strats_for_shortcode_with_results(shortcode, num_strats_to_find, last_date, lookback=100):
    """

     Get list of strats for shortcodes along with their results for given number of days

    :param shortcode:
    :param num_strats_to_find:
    :param last_date:
    :param lookback:
    :return: strat_to_date_to_results

    """
    strat_to_date_to_results = {}
    stratlist = get_strats_for_shortcode(shortcode)
    stratlist = stratlist[:num_strats_to_find]

    for strat in stratlist:
        base_strat = os.path.basename(strat)

        # get last 10 days result for the shortcode
        show_results_cmd = [execs().show_recent_global_results, shortcode, base_strat, '10']
        results = subprocess.check_output(show_results_cmd).splitlines()

        for resultline in results:
            resultline = resultline.split()
            # find valid day wise result lines
            if len(resultline) > 5 and resultline[0] != 'STATISTICS':
                date = resultline[0]

                # the format of resultline
                # 20170309 273 49000 Turn: 372, 717 norm-tcc 549 min: -14 max: 290 draw: 66 zs: 9.41 S: 29 B: 46
                # A: 23 I: 0 apos: 770.60 msgs: 7734 otl_hit: 0 abs_op_pos: 0.00 uts: 500 ptrds: 15 ttrds: 24

                # add results to map
                if strat in list(strat_to_date_to_results.keys()):
                    strat_to_date_to_results[strat][date] = resultline
                else:
                    strat_to_date_to_results[strat] = {}
                    strat_to_date_to_results[strat][date] = resultline

    return strat_to_date_to_results
