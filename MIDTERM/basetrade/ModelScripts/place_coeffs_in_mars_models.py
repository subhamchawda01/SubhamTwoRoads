#!/usr/bin/python

import os
import sys

# ARGS: <indicatorlist file> <mars_reg_out_file> <model_file_name>
if len(sys.argv) < 4:
    print "{0} <ilist_file> <mars_reg_out_file> <model_File_name".format(sys.argv[0])
    exit(0)

model_file = open(sys.argv[1], 'w')
ilist_file = open(sys.argv[2])
mars_reg_file = open(sys.argv[3])

model_footer_ = ""


class Term1:  # for deg2 MARS
    def __init__(self, s):
        ind_line = s.strip().split()
        self.degree_ = int(ind_line[0])
        self.beta_ = float(ind_line[1])
        self.basis_funcs_ = [[int(ind_line[2 + i * 3]), float(ind_line[3 + i * 3]),
                              int(ind_line[4 + i * 3])] for i in range(self.degree_)]

    def addInd(self, indstr, index):
        self.inds_ = indstr

    def setIndex(self, ind):
        self.index_ = ind

    def __str__(self):
        return "{0} {1} {2}" . format(self.degree_, self.beta_, self.basis_funcs_)


class Term:  # constrained MARS to our settings
    def __init__(self, s):
        ind_line = s.strip().split()
        self.index = int(ind_line[0])
        self.knot_beta = [float(x) for x in ind_line[1:5]]
        self.knotIndices = ind_line[7]

    def setIndex(self, ind):
        self.index_1 = ind


def getIndicator(s):
    t = s.strip().split()
    return ' '.join(t[2:]);


model_init = ilist_file.readline()
model_math = "MODELMATH NONLINEAR CHANGE\n"
# header starts
model_file.write(model_init)
model_file.write(model_math)
model_file.write("INDICATORSTART\nINTERCEPT 0.000\n")
# header ends

mars_reg_file_data = [l.strip() for l in mars_reg_file.readlines() if len(l.strip()) > 0]
Model = [Term(l.strip()) for l in mars_reg_file_data if l[0] != '#']
model_footer_ = '\n'.join([l.strip() for l in mars_reg_file_data if l[0] == '#'])

indicators = [getIndicator(l.strip()) for l in ilist_file.readlines() if "INDICATOR " in l]
model_ind = []
for m in Model:
    if m.index not in model_ind:
        model_ind.append(m.index)
# print model_ind;
model_indicators = [indicators[i] for i in model_ind]
mdl_index = dict()
for i in range(len(model_ind)):
    mdl_index[model_ind[i]] = i

model_file.write('\n'.join(["INDICATOR 1.00 %s" % x for x in model_indicators]) + '\n')
for m in Model:
    s = ""
    i = mdl_index[m.index]
    ind = model_indicators[i].split()[0]
    model_file.write("NONLINEARCOMPONENT %f %s %d %f -1\n" %
                     (m.knot_beta[1], ind, i, m.knot_beta[0]))  # right hand part
    model_file.write("NONLINEARCOMPONENT %f %s %d %f 1\n" % (m.knot_beta[3], ind, i, m.knot_beta[2]))  # left hand part

model_file.write("INDICATOREND\n")
model_file.write(model_footer_ + "\n")
model_file.close()
# print open(model_file).read()
