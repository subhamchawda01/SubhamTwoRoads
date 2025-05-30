#!/usr/bin/env python

"""
Dumps structured config to database

"""

import sys

import MySQLdb

from walkforward.definitions.structured_config import StructuredConfig
from walkforward.wf_db_utils.dump_config_to_db import dump_config_to_db


def dump_structured_config_to_db(config, overwrite=False):
    """
    Dump a structured strat to database.
    because the structured and sub-strats are in same table. 
    it iteratively calls dump config_to_db
    
    :param config: 
    :return: 
    """
    if not config.is_valid_config(False):
        sys.stderr.write('Structured Config is not Valid')
        return -1, -1

    (exists, configid) = dump_config_to_db(config, overwrite)

    if configid < 1:
        sys.stderr.write( "Couldn't insert the config to database")
        return -1, -1

    for sub_config in config.config_vector:
        sub_config.structured_id = configid
        (val, sub_configid) = dump_config_to_db(sub_config, overwrite)
        if sub_configid <= 1:
            sys.stderr.write('Could not insert the subconfig to database' + sub_config.cname)
            return -1, -1

    return exists, configid
