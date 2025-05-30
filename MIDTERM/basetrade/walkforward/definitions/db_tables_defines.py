#!/usr/bin/env python

"""
Defines the tables as classes, and indices as class variables

"""


wf_configs_search_query = "SELECT configid, cname, shortcode, execlogic, start_time, end_time, sname, strat_type, event_token, " \
                          "query_id, config_type, config_json, simula_approved, type, description, pooltag, tstamp, expect0vol," \
                          " is_structured, structured_id FROM  wf_configs WHERE "

wf_configs_insert_query = 'INSERT INTO wf_configs ( cname, shortcode, execlogic, start_time, end_time, strat_type, ' \
                          'event_token, query_id, config_type, config_json, simula_approved, type, description, pooltag,' \
                          ' expect0vol, is_structured, structured_id ) VALUES '

wf_configs = {'configid': 0, 'cname': 1, 'shortcode': 2, 'execlogic': 3, 'start_time': 4, 'end_time': 5,
              'sname' : 6, 'strat_type': 7, 'event_token': 8, 'query_id': 9, 'config_type': 10, 'config_json': 11,
              'simula_approved': 12, 'type': 13, 'description': 14, 'pooltag': 15, 'tstamp': 16, 'expect0vol': 17,
              'is_structured': 18 ,'structured_id': 19}
