"""

"""


from pylib.execute_db_cmd import execute_db_cmd


def dump_runtime_profiler_stats_to_db(tradingdate):
    query_id = 1

    # create table separating the different stats
    insert_cmd = 'INSERT INTO t2t_data () VALUES () WHERE tradingdate = %d ' % (tradingdate)

    results = execute_db_cmd(insert_cmd)

    # for now
    return results