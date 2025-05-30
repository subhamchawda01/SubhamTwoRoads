import numpy as np
import time
from pylib.pnl_modelling_utils.summarize_results_choose_strat import summarize_results_and_choose
from pylib.pnl_modelling_utils.weights_util import *
from pylib.pnl_modelling_utils.utils import *
from pylib.pnl_modelling_utils.run_sim import run_sim
from pylib.pnl_modelling_utils.k_fold_validation import *


def three_step_optim(ilist, num_indicators, logfilename, max_val=4, num_weights=200):
    '''

    Perform greedy search over indicator x weight space. 
    In the initial iteration all indicator weight conntribution is equally likely to be sampled. In the subsequent iterations the indicator
    sampling probablity is changed based on what weight the indicator takes in the top strategies in the last iteration. Total of three iteration
    is run in three step optim.


    shortcode:str
                Product shortcode 

    execlogic:str
                Trading logic

    ilist: str
                Full path of the ilist file

    param_file_list: 
                List of param files

    training_start_date: str
                The start date to start running three step optim from.

    training_end_date: str
                The end date till which point to run pnl modelling to,

    prev_date:  str
                Validation end date


    start_time: str
                The strategy start time

    end_time: str
                The strategy end time

    num_indicators: int
                The number of indicators in the ilist file.



    return:

            weights: 2D Array    num_indicator x max_allowed_weight
                    The final probablity matrix , where entry [i,j] is probab of indicator in row i taking weight in column j

            scaled_weights: 2D Array    num_indicator x max_allowed_weight
                    The 


    '''

    # max contribution of an indicator can be max_val (4 by default)
    prob_distribution = np.ones((num_indicators, max_val))

    # in the first iteration all indicator weight combination are equally likely to be sampled
    prob_distribution /= max_val

    logfile = open(logfilename, 'a')

    logfile.write("Generating weights with the probability distribution\n")
    logfile.close()
    logfile = open(logfilename, 'ab')
    np.savetxt(logfile, prob_distribution, fmt='%.2f')
    logfile.close()

    prob_distribution = prob_distribution.tolist()

    # generate random weight sampling from the given probab distribution.

    weights = generate_random_weights_with_prob_and_distance(
        num_weights, num_indicators, prob_distribution, max_val)

    weights = remove_all_zero_combination(weights.tolist())
    weights = np.array(weights)

    enforce_sign_check(ilist, weights)

    logfile = open(logfilename, 'a')
    logfile.write("New Weights generated: " + str(weights.shape[0]) + "\n")
    logfile.close()

    return np.array(weights)
