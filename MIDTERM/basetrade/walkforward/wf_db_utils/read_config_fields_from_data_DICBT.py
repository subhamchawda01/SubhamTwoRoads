#!/usr/bin/env python

"""
Just update the config struct from data field

"""

from walkforward.definitions.db_tables_defines import wf_configs


def read_config_fields_from_data_DICBT(config_struct, line, shortcode):
    """
    looks messy but it at one place compared to 10 places in past

    :param config_struct: to be filled with values
    :param line: output from db query
    :return:
    """
    (config_struct.configid, config_struct.cname, config_struct.shortcode, config_struct.execlogic,
     config_struct.start_time,
     config_struct.end_time, config_struct.strat_type, config_struct.event_token, config_struct.query_id,
     config_struct.config_type, config_struct.config_json, config_struct.simula_approved, config_struct.type,
     config_struct.description, config_struct.pooltag, config_struct.expect0vol, config_struct.is_structured,
     config_struct.structured_id) \
        = (int(line[wf_configs['configid']]), line[wf_configs['cname']], shortcode,
           line[wf_configs['execlogic']],
           line[wf_configs['start_time']], line[wf_configs['end_time']], line[wf_configs['strat_type']],
           line[wf_configs['event_token']], int(line[wf_configs['query_id']]), int(line[wf_configs['config_type']]),
           line[wf_configs['config_json']], line[wf_configs['simula_approved']], line[wf_configs['type']],
           line[wf_configs['description']], line[wf_configs['pooltag']], int(line[wf_configs['expect0vol']]),
           int(line[wf_configs['is_structured']]), int(line[wf_configs['structured_id']]))

    return config_struct
