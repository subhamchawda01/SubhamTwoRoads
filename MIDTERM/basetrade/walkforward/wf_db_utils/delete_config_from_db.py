#!/usr/bin/env python

"""
Deletes the config from db,
if flag set, then removes the results as wel
"""

from __future__ import print_function

from walkforward.wf_db_utils.cascade_delete import clear_configid_contents_from_schema, delete_config_from_configid
from walkforward.wf_db_utils.fetch_config_details import fetch_config_id


def delete_config_from_db(configname):
    """
    Removes the config form the wf_config table , strats from the wf_strats table; results from the wf_results table

    configname:str
                The config name to be removed

    returns: 0 (success) / 1 (failed) [the return value is not correctly implemented yet]

    """

    config_id = fetch_config_id(configname)

    if not config_id or config_id == -1:
        print(("count not fetch configid for %s" % (configname)))
        return 1

    # delete config contents from models, params, wf_strats, and then from wf_configs
    delete_config_from_configid(config_id)

    return 0
