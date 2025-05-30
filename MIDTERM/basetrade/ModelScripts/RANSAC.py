#!/usr/bin/env python
import numpy as np
from sklearn import linear_model, datasets
import sys


def MAD(data, axis=None):
    return np.median(np.absolute(data - np.median(data, axis)), axis)


def random_partition(n, n_data):
    all_idxs = np.arange(n_data)
    np.random.shuffle(all_idxs)
    idxs1 = all_idxs[:n]
    idxs2 = all_idxs[n:]
    return idxs1, idxs2


def get_error(model, X, Y):
    return np.absolute(np.subtract(Y, model.predict(X)))


def RANSAC(model):
    iterations = 0
    bestfit = None
    besterr = np.inf
    best_inlier_idxs = None
    n = int(0.05 * X.shape[0])
    k = 2500
    t = MAD(Y)
    d = int(0.45 * X.shape[0])
    print "Threshold=", d
    while iterations < k:
        maybe_idxs, test_idxs = random_partition(n, X.shape[0])
        maybeinliers_ind = X[maybe_idxs, :]
        maybeinliers_dep = Y[maybe_idxs]
        test_points_ind = X[test_idxs]
        test_points_dep = Y[test_idxs]
        maybemodel = model.fit(maybeinliers_ind, maybeinliers_dep)
        test_err = get_error(maybemodel, test_points_ind, test_points_dep)
        also_idxs = test_idxs[test_err < t]  # select indices of rows with accepted points
        alsoinliers_ind = X[also_idxs, :]
        alsoinliers_dep = Y[also_idxs]
        if alsoinliers_dep.shape != () and alsoinliers_dep.shape[0] > d:
            betterdata_ind = np.concatenate((maybeinliers_ind, alsoinliers_ind))
            betterdata_dep = np.concatenate((maybeinliers_dep, alsoinliers_dep))
            bettermodel = model.fit(betterdata_ind, betterdata_dep)
            bettererrors = get_error(model, X, Y)
            if np.median(bettererrors) <= besterr:
                bestfit = bettermodel
                besterr = np.median(bettererrors)
        iterations += 1
    if bestfit is None:
        raise ValueError("did not meet fit acceptance criteria")
    return bestfit


if len(sys.argv) < 2:
    print "Usage:<script> <filtered regdata file>"

X = []
Y = []
with open(sys.argv[1]) as file:
    for line in file:
        line = line.strip().split()
        X.append([])
        for indicator in line[1:]:
            X[len(X) - 1].append(float(indicator))
        Y.append(float(line[0]))
X = np.array(X)
Y = np.array(Y)

model = linear_model.Ridge(alpha=0.25, normalize=True)

model = RANSAC(model)

print "Score=", model.score(X, Y)
print("Estimated coefficients (RANSAC):")
print(model.coef_)
