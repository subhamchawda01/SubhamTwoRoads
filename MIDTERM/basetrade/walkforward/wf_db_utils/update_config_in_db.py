#!/usr/bin/env python

"""
Updates the data for existing config fields
"""

from __future__ import print_function

import MySQLdb

from walkforward.wf_db_utils.db_handles import connection


def update_config_in_db(configname, cfg):
    """

    Update the config entry in the wf_config table

    configname: str
                the name of the config that has to be udpated

    cfg: config_struct
                the config_struct that has to be updated to the table


    return: 0 (success) / 1 (failure)


    """
    insert = True

    if not cfg.is_valid_config(True):
        print('Config created is not valid')
        return 1

    if not cfg.check_modelparam_exists_in_fs():
        print('Config created is not valid: some of the models/params do not exists in filesystem')
        return 1

    if not cfg.cname:
        print("Config name not present")
        return 1

    cursor = connection().cursor()

    if cfg.event_token == 'INVALID' and cfg.strat_type == 'EBT':
        raise ValueError("invalid config " + cfg.cname + " with strat_type: " +
                         cfg.strat_type + " and event_token: " + cfg.event_token)

    if cfg.event_token != 'INVALID' and cfg.strat_type == 'Regular':
        raise ValueError("invalid config " + cfg.cname + " with strat_type: " +
                         cfg.strat_type + " and event_token: " + cfg.event_token)

    update_query = ('UPDATE wf_configs SET cname = \"%s\", shortcode = \"%s\", execlogic = \"%s\", start_time = \"%s\",'
                    ' end_time = \"%s\", strat_type = \"%s\", event_token = \"%s\", query_id = %d,'
                    ' config_type = %d, config_json = \'%s\', simula_approved = %d, type = \"%s\", '
                    'description = \"%s\", pooltag = \"%s\", expect0vol = %d, is_structured = %d, structured_id = %d'
                    ' WHERE configid = %d' % (configname, cfg.shortcode, cfg.execlogic, cfg.start_time, cfg.end_time,
                                              cfg.strat_type, cfg.event_token, cfg.query_id, cfg.config_type,
                                              cfg.config_json, cfg.simula_approved, cfg.type, cfg.description,
                                              cfg.pooltag, cfg.expect0vol,  cfg.is_structured,
                                              cfg.structured_id, cfg.configid))

    try:
        cursor.execute(update_query)
        return 0
    except MySQLdb.Error as e:
        print(("FAILED: " + update_query, e))
        # connection().rollback()
        return 1
