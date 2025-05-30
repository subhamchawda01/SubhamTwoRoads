#!/usr/bin/env python
import sys
from sklearn import linear_model


def main():
    if (len(sys.argv)) < 3:
        print("USAGE: " + sys.argv[0] + " <regdatafile> <regoutfile> ")
        exit()

    X = []
    Y = []
    regdatafilename_ = sys.argv[1]
    regoutfilename_ = sys.argv[2]
    with open(regdatafilename_) as infile:
        for line in infile:
            line = line.split()
            line = list(map(float, line))
            Y.append(line[0])
            X.append(line[1:len(line)])

    alpha_vec_ = [0.0001, 0.001, 0.01, 0.1, 0.3, 1, 10]
    model_ = linear_model.RidgeCV(alphas=alpha_vec_)
    model_.fit(X, Y)
    print(model_.coef_)
    print(model_.alpha_)

    outfile_ = open(regoutfilename_, 'w')
    count = 1
    for line in model_.coef_:
        outfile_.write(str(count) + " " + str(line) + "\n")
        count = count + 1

    outfile_.close()

    for alpha_ in alpha_vec_:
        print("FOR: " + str(alpha_) + "\n RIDGE")
        model_ = linear_model.Ridge()
        model_.set_params(alpha=alpha_)
        model_.fit(X, Y)
        print(model_.coef_)

    print("NORIDGE: ")
    model_ = linear_model.LinearRegression()
    #model_.set_params ( alpha=alpha_ )
    model_.fit(X, Y)
    print(model_.coef_)
    # print model_.alpha_


if __name__ == "__main__":
    main()
