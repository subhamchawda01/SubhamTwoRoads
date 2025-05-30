#!/usr/bin/python

from .pythonUtils import *
from multiprocessing import Pool

uid = random.randint(10000, 99999)


def ParseCommandLine():
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option("-X", "--execs", dest="X", help="execs you want to compare", nargs=2)
    parser.add_option("-M", "--modelfile", dest="M", help="model file you want to test")
    parser.add_option("-P", "--param",  dest="P", default="EST_430 EST_1430 %d 3000 1 1 0" % uid,
                      help="datagen parameters")
    parser.add_option("-d", "--date",  dest="dt", help="the date on which datagen will be run", default="20130405")
    (options, args) = parser.parse_args()
    return options


options = ParseCommandLine()
# print options
params = options.P.split()
modelfl = getModelsFromStrat(options.M)[0]


def runDatagen(s):
    exe = s[0]
    outfl = s[1]
    global options
    datagen_cmd = "{datagen_binary_} {model_fl_} {date_} {params1_} {out_fl_} {params2_} ADD_DBG_CODE -1".format(
        datagen_binary_=exe, model_fl_=modelfl, out_fl_=outfl,
        date_=options.dt, params1_=' '.join(params[:3]), params2_=' '.join(params[3:]))
    if not (os.path.exists(outfl)):
        print(RunCommand(datagen_cmd)[1])
    else:
        print(outfl, 'exists. remove and rerun.')


pool = Pool()
md5_sums = [RunCommand("md5sum %s | awk '{print $1}'" % e)[0][:6] for e in options.X]
data_out_files = ["%s/data_out_%s" % (home_dir, m) for m in md5_sums]
t = list(zip(options.X, data_out_files))

pool.map(runDatagen, t)
t = compDatagen(data_out_files[0], data_out_files[1])  # returns indicator indices where the files differ

if len(t) > 0:
    if t[0] == -1:
        print("ERROR!!!")
        exit()
    inds = getIndicatorsFromModel(modelfl)
    print("\nDatagen_Cur_out: %s\t(Please delete this after you are done with debuggin)\nDatagen_orig_out: %s\n" %
          (datagen_out_file, master_output))
    print(">>>>Datagen DIFFERS at these indicators: ")
    for i in t:
        print(inds[i])
else:
    print("Congrats!! datagens have exact match with each other")
    for f in data_out_files:
        os.remove(f)
