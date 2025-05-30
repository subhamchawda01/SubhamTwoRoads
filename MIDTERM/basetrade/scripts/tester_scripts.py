#!/usr/bin/python

from .pythonUtils import *
import re
import os
import json
import random
from multiprocessing import Pool

# To create random test mode sets.
# a="FGBM_0 FESX_0 FOAT_0 LFI_0 LFZ_0 KFFTI_0 LFR_0 ZF_0 ZB_0 ZN_0 UB_0 BAX_3 CGB_0 BR_IND_0 BR_DOL_0 BR_WIN_0 DI1F18 HHI_0 NK_0 HSI_0 NKM_0"
# for i in $a; do for c in 0 1 6 20 45; do ~/gen_model.sh $i $c; done ; done
#

st_db_file_dir = '/spare/local/tester_db/'

strat_db = open('%s/st_list.txt' % st_db_file_dir, 'rb')
db = json.load(strat_db)
DATE = '20130529'
strat_types = ["LONG", "VERYLONG", "EMPTY", "SHORT"]
sim_strat_exec = "%s/sim_strategy" % bin_path
update = False

#print ( json.dumps(db, sort_keys=True, indent=2, separators=(',',': ')) )
# for E in db:
#     print E
#     for P in db[E]:
#         print '---', P["name"]
error_percentage_allowed = 0.0


def matchSIMRes(res, db_res):
    db_res = [x for x in db_res.split('#') if x]
    res = [x for x in res.split('#') if x]
    if len(res) != len(db_res):
        return False
    for x, y in zip(res, db_res):
        c_pnl, c_vol = [int(r) for r in x.split()[1:3]]
        d_pnl, d_vol = [int(r) for r in y.split()[1:3]]
        if c_vol + d_vol == 0:
            c_vol = 1
            d_vol = 1
        if abs(d_pnl - c_pnl) * 2.0 / (abs(d_pnl) + abs(c_pnl) + 1) + abs(d_vol - c_vol) * 2.0 / (d_vol + c_vol) > error_percentage_allowed / 100.0:
            return False
    return True


def RunSimAndGetRes(st):
    if not st:
        return "-NO STRAT-"
    uid = random.randint(1, 99999)
    cmd = "%s SIM %s %d %s" % (sim_strat_exec, st[0], uid, st[1])
    res = RunCommand(cmd)[0].strip().replace('\n', '#')
    RunCommand("rm /spare/local/logs/tradelogs/*.%s.%d" % (DATE, uid))
    if update or len(st) < 3:
        # update DB
        return res
    if matchSIMRes(res, st[2]):
        print("Matched : ", res, st[2])
        return 1
    else:
        print("Failed @ ", st, '\nCurrent output:', res);
        return 0


def RunSim(arg):
    s = arg[0]
    t = arg[1]
    if not s["path"]:
        return "-NO-STRAT-"
    uid = random.randint(1, 99999)
    cmd = "%s SIM %s %d %s" % (sim_strat_exec, s["path"], uid, t)
    res = RunCommand(cmd)[0].strip().replace('\n', '#')
    RunCommand("rm /spare/local/logs/tradelogs/*.%s.%d" % (DATE, uid))
    return res


def chckOK(stList):  # takes a list of strategy-result pairs and runs strategy and match res
    isOk = True
    pool = Pool(12)  # default works better !! TODO: Test
    l = []
    for s in stList:
        for t in s["expected_output"]:
            l.append((s["path"], t, s["expected_output"][t]))
    simresults = pool.map(RunSimAndGetRes, l)
    return not 0 in simresults


def getStList(prod):
    l = []
    for typ in prod:
        if typ in ['name', 'type']:
            continue
        s = prod[typ]
        for t in s["expected_output"]:
            l.append((s["path"], t))
    return l


def setStList(prod, l):
    i = 0
    for typ in prod:
        if typ in ['name', 'type']:
            continue
        s = prod[typ]
        for t in s["expected_output"]:
            s["expected_output"][t] = l[i]
            i += 1


def updateDB(Exch="ALL"):
    """
    update the sim result in the DB for current git version. rename the old db as pervious git version.
    """
    # git_hash = ''.join(RunCommand ( "git log --pretty=format:'%h' -n 1" )[0]).strip();
    exec_hash = RunCommand('md5sum %s' % sim_strat_exec)[0][:7]
# backup curent version
    hashed_file_name = "%s/st_list.json" % (st_db_file_dir)
    if Exch == "ALL":
        hashed_file_name = "%s/st_list_%s.json" % (st_db_file_dir, exec_hash)
        if os.path.isfile(hashed_file_name):  # already exist just copy
            print(hashed_file_name, "already exist. Remove for force update")
            RunCommand("cp %s %s/st_list.txt" % (hashed_file_name, st_db_file_dir))
            return
        print("You are going to update DB. Current version will be backed up with _%s_back name." % exec_hash)
        RunCommand("cp %s/st_list.txt %s/st_list_%s_bak.txt" % (st_db_file_dir, st_db_file_dir, exec_hash))

    pool = Pool()
    for E in db:
        if ((Exch != "ALL") and (Exch != E)):
            continue
        print(E)
        if E == "OTHER":
            setStList(db["OTHER"], pool.map(RunSimAndGetRes, getStList(db["OTHER"])))
            continue
        l = [(prod[typ], t)
             for prod in db[E]
             for typ in prod if typ not in ['name', 'type']
             for t in prod[typ]["expected_output"]
             ]
        # print json.dumps(l, indent=2);
        # simresults
        results = pool.map(RunSim, l)
        for st, res in zip(l, results):
            st[0]["expected_output"][st[1]] = res
        # datage resutls
        print("Updating Datagen DBs")
        pool.map(testDatagen, [x for x in set([s[0]["path"] for s in l]) if x.find("long") >= 0])
    with open(hashed_file_name, 'wb') as db_file_with_hash:
        json.dump(db, db_file_with_hash, sort_keys=True, indent=2, separators=(',', ':'))
    if (Exch == "ALL"):
        RunCommand("cp %s %s/st_list.txt" % (hashed_file_name, st_db_file_dir))


def syncDB():
    RunCommand("for s in %s/datagen_out_model_*_orig; do rm ${s}.gz" % st_db_file_dir)
    RunCommand("/bin/gzip %s/datagen_out_model_*_orig" % st_db_file_dir)
    RunCommand("~/basetrade/scripts/sync_dir_to_all_machines.pl %s" % st_db_file_dir)


def initializeDB(Exch="ALL"):
    """
    to create the st_list.txt file
    """
    from glob import glob
    from copy import deepcopy
    a = {}
    dates = ["20130506", "20130622", "20130529"]
    # to initialize with own dates - change here
    #
    dates += [dt for dt in itertools.islice(date_generator(today), 10)]  # 20130530 to last 10 weekdays;
    print(dates)
    #######################

    a["EUREX"] = [("FGBM_0", "MEDIUM"), ("FESX_0", "FAST"), ("FOAT_0", "MEDIUM"),
                  ("FBTS_0", "MEDIUM"), ("FDAX_0", "FAST"), ("FGBS_0", "SLOW")]
    a["LIFFE"] = [("LFI_0", "SLOW"), ("LFZ_0", "MEDIUM"), ("KFFTI_0", "FAST"), ("LFR_0", "FAST")]
    a["CME"] = [("ZF_0", "SLOW"), ("ZB_0", "MEDIUM"), ("ZN_0", "SLOW"), ("UB_0", "MEDIUM")]
    a["TMX"] = [("BAX_3", "SLOW"), ("CGB_0", "FAST")]
    a["BMF"] = [("BR_IND_0", "FAST"), ("BR_DOL_0", "FAST"), ("BR_WIN_0", "FAST"), ("DI1F18", "FAST")]
    a["HK"] = [("HHI_0", "FAST"), ("HSI_0", "FAST")]
    a["OSE"] = [("NK_0", "FAST"), ("NKM_0", "FAST")]

    for e in a:
        if (Exch != "ALL") and (e != Exch):
            continue
        if e not in db:
            db[e] = []
        del db[e][:]
        for p in a[e]:
            s = {"name": p[0], "type": p[1]}
            st_for_p = [x for x in glob("%s/st_%s_*" % (st_db_file_dir, p[0])) if '~' not in x]
            for st in st_for_p:
                ret = re.sub(r'.*_([a-z]*)', r'\1', st).upper()
                s[ret] = {"expected_output": {}, "path": st}
                for dt in dates:
                    if dt not in s[ret]["expected_output"]:
                        s[ret]["expected_output"][dt] = ""
            db[e].append(deepcopy(s))
            # print json.dumps( s, sort_keys=True, indent=2, separators=(',',': '))
    db_file = '%s/st_list.txt' % (st_db_file_dir)
    with open(db_file, 'wb') as db_f:
        json.dump(db, db_f, sort_keys=True, indent=2, separators=(',', ':'))


def getProdsOfType(E, t, s=None):
    if E not in db:
        print("Could not find exchange: ", E); exit(0);
    if not s:
        return [p for p in db[E] if p["type"] == t]
    else:
        return [prod[s]["path"] for prod in [p for p in db[E] if p["type"] == t and s in p]]


def testLevel1(E="ALL"):  # E - EXchange, P - Product, I - Indicator
    """
    select 1 fast and 1 slow moving prod and run there 'LONG' and 'EMPTY' model and check the result
    """
    t = []
    passed = True
    if E == "ALL":
        t = list(db.keys())
    elif E in db:
        t = [E]
    else:
        print("Could not find exchange: ", E); exit(0);
    for e in t:
        if e == "OTHER":
            continue
        print("Level1 Testing for ", e, " ==> ")
        p_list = []
        prods = getProdsOfType(e, "FAST")
        if len(prods) > 0:
            p_list.append(random.choice(prods))
        prods = getProdsOfType(e, "SLOW")
        if len(prods) > 0:
            p_list.append(random.choice(prods))
        print(', '.join([p["name"] for p in p_list]))
        if (chckOK([p[t] for p in p_list for t in ["EMPTY", "LONG"] if t in p])):
            print("Passed")
        else:
            passed = False
            print("Failed")
    if passed:
        print("AWESOME!!! Level1 check passed!!\n")
    else:
        print("Sorry. But Level1 check failed. See previous errors and go for datagen and Inidcator test for further debug")


def testLevel2(E="ALL"):
    """
    test for all products for this exchange with there 'LONG' and 'EMPTY' model and check the result
    """
    t = []
    passed = True
    if E == "ALL":
        t = list(db.keys())
    elif E in db:
        t = [E]
    else:
        print("Could not find exchange: ", E); exit(0);
    for e in t:
        if t == "OTHER":
            continue
        print("Testing for ", e, " ==> ", end=' ')

        if (chckOK([p[t] for p in db[e] for t in ["EMPTY", "LONG"] if t in p])):
            print("Passed")
        else:
            passed = False
            print("Failed")
    if passed:
        print("AWESOME!!! Level2 check passed!!\n")
    else:
        print("Sorry. But Level2 check failed. See previous errors and go for datagen and Inidcator test for further debug")


def testLevel3(I="ALL"):
    """
    test OTHERS
    """
    t = list(db["OTHER"].keys())
    if I != "ALL":
        t = [x for x in t if I in x]
    if not t:
        print("Could not find any type of %s in DataBase." % str(I))
        return
    print("Tesing Custom TestCases for: ", t,  "===>")
    if (chckOK([db["OTHER"][x] for x in t])):
        print("AWESOME!!! Level3 check passed!!\n")
    else:
        print("Sorry. But Level3 check failed. See previous errors and go for datagen and Inidcator test for further debug")


def testDatagen(st):
    global update
    modelfl = getModelsFromStrat(st)[0]
    print(modelfl)
    inp_date = 20130607  # ??
    res = ""
    res = "## Testing Datagen for %s ##\n" % modelfl
    datagen_out_file = "%s/datagen_out_%s_curr" % (st_db_file_dir, os.path.basename(modelfl))
    datagen_cmd = "{bindir_}/datagen {model_fl_} {date_} CET_600 CET_1600 25452 {out_fl_} 2000 0 0 0 ADD_DBG_CODE -1".format(
        bindir_=bin_path, model_fl_=modelfl, out_fl_=datagen_out_file, date_=inp_date)
    RunCommand(datagen_cmd)
    master_output = "%s/datagen_out_%s_orig" % (st_db_file_dir, os.path.basename(modelfl))
    if update:
        RunCommand("mv %s %s" % (datagen_out_file, master_output))
        return
    t = compDatagen(datagen_out_file, master_output)  # returns indicator indices where the files differ
    if len(t) > 0:
        if t[0] == -1:
            res += "I/O ERROR!!"
            return res
        inds = getIndicatorsFromModel(modelfl)
        res += "\nDatagen_Cur_out: %s\t(Please delete this after you are done with debuggin)\nDatagen_orig_out: %s\n" % (
            datagen_out_file, master_output)
        res += ">>>>Datagen DIFFERS at these indicators: "
        for i in t:
            res += inds[i] + "\n"
    else:
        res += "Congrats!! datagen has exact match with master"
        os.remove(datagen_out_file)
    return res


def testLevel4(E="ALL"):  # for all
    """
    select one long verylong model run datagen and compare
    """
    t = []
    if E == "ALL":
        t = list(db.keys())
    elif E in db:
        t = [E]
    else:
        print("Could not find exchange: ", E); exit(0);
    p_list = []
    for e in t:
        if e == "OTHER":
            continue
        print("Level4(datagen) Testing for ", e, " ==> ")
        prods = getProdsOfType(e, "FAST", "LONG")
        if len(prods) > 0:
            p_list.append(random.choice(prods))

    print(p_list)
    pool = Pool()
    results = pool.map(testDatagen, p_list)
    print('\n\n'.join(results))


def ParseCommandLine():
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option("-b", "--branch", dest="B",
                      help="the branch you want to compare, devmodel, devtrade( Not Implemeted: comapres with master )", metavar="BRANCH_NAME")
    parser.add_option("-L", "--level",  dest="L", default=0, help="testing level, currently 3 leveels allowd, : 1,2,3")
    parser.add_option("-E", "--exchange",  dest="E", default="ALL", help="EUREX, LIFFE etc")
    parser.add_option("-B", "--error-limit",  dest="B", default="0", help="percentage eror allowed.")
    parser.add_option("-C", "--custom-strat-type-name",  dest="C", help="custome strats that you specifially want to test. like BOOK_IND, TREND_IND etc, \
these are of OTHER Group. Feel free to add any strategy-res element and test. Will ignore Level options.")
    # parser.add_option("-X", "--execs", dest="X", help="specify the execs path separated by colon( no-space ) to compare results. "
    (options, args) = parser.parse_args()
    return options


def main():
    global update
    usage = """ 
This is a testing script that matches the sim_pnl( currently only sim_pnl ) some chosen strats
and try to help you find out any unwanted changes made to the code base or know the effect/side effect of your changes. Compile and have your sim_strategy exec in /home/%s/basetrade_install/bin/. 
Arguments:
   -b <branch-name> # the branch you want to compare ( Till Not Implemented ).
   -L <test level> # currently 3 levels supported ( 1 - 3 )
   -E <exchange> # EUREX, LIFFE, for which you want specifially to run. default level is 2 for this argument.
   -C <custome type name> # custome strats that you specifially want to test. like BOOK_IND, TREND_IND etc.
                            these are of OTHER Group. Feel free to add any strategy-res element and test.
   -h Help script
InJoyaay.. 
""" % user
    special_options = """~~~SPECIAL OPTIONS~~~ 
-L u -> to update DB
-L U -> to update DB and syc to all ny, crt
-L i -> initialize DB with the strats available in the /spare/loca/tester_db
( all optinos accept the -E <exchagne> option. )
"""

    options = ParseCommandLine()
    print(options)

    # No argumemt - Help message
    if not options.C and options.L == 0:
        print(usage)
        if user in ["rahul", "gchak", "rakesh"]:
            print(special_options)
        return

    # update DB arguments
    if options.L in ['u', 'U']:
        update = True
        updateDB(options.E)
        if options.L == 'U':
            syncDB()
        return

    # Initialize DB arguments
    if options.L == 'i':
        initializeDB(options.E)
        return

    global error_percentage_allowed
    error_percentage_allowed = float(options.B)
    # testing arguments
    options.L = int(options.L)
    level_test_func = [testLevel1, testLevel2, testLevel3, testLevel4]
    if options.L < 1:
        options.L = 1
    if options.L > 4:
        options.L = 3
    if not options.C:
        level_test_func[options.L - 1](options.E)
    else:
        testLevel3(options.C)


if __file__ == sys.argv[0]:
    main()

#testLevel3 ( "BOOK_IND" )
#updateDB ( );
