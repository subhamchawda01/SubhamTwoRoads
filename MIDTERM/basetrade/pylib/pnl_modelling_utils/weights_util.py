#!/usr/bin/env python
"""
Weight utilities used in pnl_modelling
"""
import math
import numpy as np


def get_model_stdev(weight, covariance_matrix):
    """
    Given the weights , and the covariance matrix, returns the stdev of the model
    :param weight: 
    :param covariance_matrix: 
    :return: 
    """
    return math.sqrt(np.dot(np.dot(weight, covariance_matrix), weight.transpose()))


def rescale_weights(weight_grid, target_stdev_list, stdev_indicators, covariance_matrix, logfilename):
    """
    Given the set of all weights, and the list of target stdev, it rescales all the weights to the target stdevs. 
    :param weight_grid: 
    :param target_stdev_list: 
    :param stdev_indicators: 
    :param covariance_matrix: 
    :param logfilename: 
    :return: 
    """
    new_weights = []
    logfile = open(logfilename, 'a')
    logfile.write("GENERATING WEIGHTS\n")
    for target_stdev in target_stdev_list:
        for weight in weight_grid:
            logfile.write("Relative weights(normalized indicators): " + ' '.join(map(str, weight)))
            logfile.write("\n")
            weight = np.divide(weight, stdev_indicators)
            logfile.write("Actual weights: " + ' '.join(map(str, weight)))
            logfile.write("\n")
            stdev = get_model_stdev(weight, covariance_matrix)
            logfile.write("Model Stdev: " + str(stdev))
            logfile.write("\n")
            weight = weight * target_stdev / stdev
            logfile.write("Rescaled weights to target stdev: " + ' '.join(map(str, weight)))
            logfile.write("\n")
            new_stdev = get_model_stdev(weight, covariance_matrix)
            logfile.write("Target Stdev: " + str(new_stdev))
            logfile.write("\n")
            logfile.write("-------------------------------------\n")
            logfile.flush()
            new_weights.append(list(weight))
    new_weights = np.array(new_weights)
    logfile.write("WEIGHTS COMPLETED\n")
    logfile.write("-------------------------------------\n")
    logfile.flush()
    logfile.close()
    return new_weights


def replicate_weights_for_stdev_list(weights, target_stdev_list):
    """
    replicate the relative weights times the target stdev list
    :param weights: 
    :param target_stdev_list: 
    :return: 
    """
    rep_wt = []
    rep_stdev = []
    for target_stdev in target_stdev_list:
        for weight in weights:
            rep_wt.append(weight)
            rep_stdev.append(target_stdev)
    return np.array(rep_wt), np.array(rep_stdev)


def get_scaled_wts_stdev_dict(actual_weights, target_stdev_list_replicated):
    """
    returns a dictionary for the actual weights and the target stdev list replicated weights
    :param actual_weights: 
    :param target_stdev_list_replicated: 
    :return: 
    """
    d = {}
    for a, s in zip(actual_weights, target_stdev_list_replicated):
        a = tuple(a)
        if a in d:
            continue
        d[a] = s
    return d


def remove_duplicate_weights(relative_weights, actual_weights):
    """
    remove the duplicate weights , which can arise by same actual weights but different relative weights like 1:1:1 will
    be same as 2:2:2
    :param relative_weights: 
    :param actual_weights: 
    :return: 
    """
    d = {}
    rel = []
    act = []
    for a, r in zip(actual_weights, relative_weights):
        a = tuple(a)
        r = tuple(r)
        if a in d:
            continue
        d[a] = r
    for a, r in d.items():
        rel.append(list(r))
        act.append(list(a))

    rel = np.array(rel)
    act = np.array(act)
    return rel, act


def remove_all_zero_combination(weights):
    """
    removing all zero weight combinations
    :param weights: 
    :return: 
    """
    non_zero_wt = [x for x in weights if np.any(x)]
    return non_zero_wt


def generate_weight_grid_from_matrix(indx, num_indicators, user_matrix):
    """
    Given a user matrix of weights, generate the possible set of combinations of weights,for the indicators
    :param indx: 
    :param num_indicators: 
    :param user_matrix: 
    :return: 
    """
    grid = []
    if indx == num_indicators - 1:
        for x in user_matrix[indx]:
            temp = [x]
            grid.append(temp)
        return grid

    temp_grid = generate_weight_grid_from_matrix(indx + 1, num_indicators, user_matrix)
    for i in range(len(user_matrix[indx])):
        for x in temp_grid:
            temp = list([user_matrix[indx][i]] + x)
            grid.append(temp)
    return grid


def generate_weight_grid(indx, num_indicators, max_sum):
    """
    Given a sum value, generate weight combinations with max weight equalling the max sum/2
    :param indx: 
    :param num_indicators: 
    :param max_sum: 
    :return: 
    """
    w = list(range(1, int(max_sum / 2) + 1))

    # w = [0, 1, 2, 3, 4, 5]
    grid = []
    if indx == num_indicators - 1:
        for x in w:
            temp = [x]
            grid.append(temp)
        return grid
    temp_grid = generate_weight_grid(indx + 1, num_indicators, max_sum)
    for i in range(len(w)):
        for x in temp_grid:
            temp = list([w[i]] + x)
            grid.append(temp)
    return grid


def check_constraints_on_weights(weight_grid, max_sum):
    """
    Only keeping weights with sum of weights equal to sum
    :param weight_grid: 
    :param max_sum: 
    :return: 
    """
    refined_weights = []
    for weight in weight_grid:
        to_add = True
        sum = 0
        for w in weight:
            sum += w
        if sum != max_sum:
            to_add = False
        if to_add:
            refined_weights.append(list(weight))
    return refined_weights


def enforce_sign_check(ilist, weights):
    """
    Enforcing sign of indicators as mentioned in ilist
    :param ilist: 
    :param weights: 
    """
    f = open(ilist, 'r')
    indx = 0
    for line in f:
        tokens = line.strip().split()
        if len(tokens) > 0 and tokens[0] == "INDICATOR":
            weight = float(tokens[1].split(":")[0])
            if weight < 0:
                weights[:, indx] = -1 * weights[:, indx]
            indx += 1


def generate_random_weights(num_weights, num_indicators):
    return np.random.randint(int(num_indicators + 1), size=(num_weights, num_indicators))


def generate_random_weights_with_prob(num_weights, num_indicators, prob_distribution, max_val):
    """
    Generating num_weights weights given the probability distribution of the possible values of the indicators
    :param num_weights: 
    :param num_indicators: 
    :param prob_distribution: 
    :param max_val: 
    :return: 
    """
    new_weights = None
    for ind in range(num_indicators):
        w = np.random.choice(max_val, num_weights, p = prob_distribution[ind])
        w = np.array(w)
        w = w.reshape(-1, 1)
        if new_weights is None:
            new_weights = w
        else:
            new_weights = np.hstack((new_weights, w))

    return new_weights

def generate_random_weights_with_prob_and_distance(num_weights, num_indicators, prob_distribution, max_val, dist_thrsld = 0.25):
    """
    Generating num_weights weights given the probability distribution of the possible values of the indicators
    :param num_weights:
    :param num_indicators:
    :param prob_distribution:
    :param max_val:
    :param dist_thrsld: threshold for distance
    :return:
    """
    max_tries = 4*num_weights
    new_weights = []
    weights_count = 0
    try_count = 0
    skips = 0
    while (weights_count < num_weights and try_count <= max_tries):
        ind_weights = []
        try_count += 1
        # Get weight for all indicators
        for ind in range(num_indicators):
            w = np.random.choice(max_val, 1, p = prob_distribution[ind])
            ind_weights.append(w[0])
        if len(new_weights) == 0:
            new_weights.append(ind_weights)
            weights_count += 1
        else:
            # Check if weight is at sufficient distance than the current weights
            distance_from_group = get_new_weight_distance(ind_weights,new_weights)
            if distance_from_group > dist_thrsld :
                new_weights.append(ind_weights)
                weights_count +=1
            else:
                skips += 1
                continue
    return np.array(new_weights)

# similar weights are defined in terms of l1 distance between them in function get_new_weight_distance
def remove_similar_distance_weights(weight, dist_thrsld = 0.25):
    new_weights = []
    for wt in weight:
        distance_from_group = get_new_weight_distance(wt,new_weights)
        if distance_from_group > dist_thrsld :
            new_weights.append(wt)
    return new_weights

def get_new_weight_distance(weight, prev_weight_matrix):
    #return 1 if prev_weights is empty currently
    if len(prev_weight_matrix) == 0:
        return 1
    counter  = 1
    # Avoid adding all 0 weight combinations
    if sum(weight) == 0:
        return 0
    # minimum_distance should be greater than the threshold.
    norm_weight = 1.0*np.array(weight)/sum(weight)
    for prev_weight in prev_weight_matrix:
        norm_prev_weight = 1.0*np.array(prev_weight)/sum(prev_weight)
        diffrence_wts = norm_weight - norm_prev_weight
        sum_wts = norm_weight + norm_prev_weight
        distance = 2.0* sum(np.abs(diffrence_wts)) / sum(sum_wts)
        if counter == 1:
            minimum_distance = distance
        counter += 1
        if distance < minimum_distance:
            minimum_distance = distance
    return minimum_distance

def perturb_weights(weight_vec, delta):
    new_weight_vec = []
    for weight in weight_vec:
        new_weight = list(weight)
        for i in range(len(new_weight)):
            new_weight[i] += delta
            new_weight_vec.append(list(new_weight))
            new_weight[i] -= 2 * delta
            new_weight_vec.append(list(new_weight))
            new_weight[i] += delta
    new_weight_vec = np.array(new_weight_vec)
    return new_weight_vec
