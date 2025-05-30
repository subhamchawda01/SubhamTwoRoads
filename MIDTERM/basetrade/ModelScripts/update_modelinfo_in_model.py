#!/usr/bin/env python
import sys


def main():
    usage = " USAGE : <script> <modelname> <norm> <stdev> "

    if (len(sys.argv) < 4):
        print usage + "\n"
        exit()

    modelfile_name_ = sys.argv[1]
    stdev_ = float(sys.argv[2])
    norm_ = float(sys.argv[3])
    model_content_ = open(modelfile_name_).readlines()
    modelinfo_string_ = "MODELINFO " + str(stdev_) + " " + str(norm_) + "\n"

    modelinfo_exists_ = False
    i = 0
    while (i < len(model_content_)):
        if (model_content_[i].split()[0].strip() == "MODELINFO"):
            modelinfo_exists_ = True

        if (model_content_[i].strip() == "INDICATORSTART" and (not modelinfo_exists_)):
            model_content_.insert(i, modelinfo_string_)
            i = i + 1

        i = i + 1

    out_file_ = open(modelfile_name_, 'w')
    out_file_.write(''.join(model_content_))

    out_file_.close()


if __name__ == "__main__":
    main()
