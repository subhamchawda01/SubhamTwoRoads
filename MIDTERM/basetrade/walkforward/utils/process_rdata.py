import subprocess
from walkforward.definitions import execs


def apply_filters(shortcode, regdata_file, ftr, filtered_file):

    filter_process_cmd = [execs.execs().apply_dep_filter, shortcode, regdata_file, ftr, filtered_file]
    process = subprocess.Popen(' '.join(filter_process_cmd), shell=True,
                               stderr=subprocess.PIPE,
                               stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')
    errcode = process.returncode

    return out, err, errcode
