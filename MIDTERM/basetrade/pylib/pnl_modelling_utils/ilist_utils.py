"""
Utility to for creating ilist and reading indicators from ilist, invoked in pnl_modelling
"""


def create_regime_ilist(regime_indicator, num_regimes, ilist, new_ilist):
    """
    Given the regime indicators, and the number of regimes, converts the given ilist into a  new ilist, with expression
    indicators of REGIME_CUTOFF
    :param regime_indicator: exact description of regime indicator
    :param num_regimes: number of regimes in regime indicator
    :param ilist: ilist to be used in pnl_modelling
    :param new_ilist: new ilist with regime indicator
    """
    indicators, weights = read_indicators_from_ilist(ilist)
    print(indicators)
    num_tokens_in_regime_indicator = regime_indicator.split()
    expression_add = []
    new_indicators = []
    new_weights = []
    for regime in range(num_regimes):
        expression_add.append("Expression REGIME_CUTOFF " + str(len(num_tokens_in_regime_indicator) -
                                                                1) + " " + str(regime + 1) + " " + regime_indicator)
    print(expression_add)
    for indicator, weight in zip(indicators, weights):
        ind_tokens = indicator.split()
        if weight > 0:
            sign = "1.00"
        else:
            sign = "-1.00"
        for expr in expression_add:
            new_indicators.append(expr + " " + str(len(ind_tokens) - 1) + " 1.00 " + indicator)
            new_weights.append(sign)
    f = open(ilist, 'r')
    pre_indicator_lines = []
    for line in f:
        tokens = line.strip().split()
        if len(tokens) > 0 and tokens[0] == "INDICATOR":
            break
        pre_indicator_lines.append(line.strip())
    f.close()
    f = open(new_ilist, 'w')
    for l in pre_indicator_lines:
        f.write(l + "\n")
    for i, w in zip(new_indicators, new_weights):
        f.write("INDICATOR " + w + " " + i + "\n")
    f.write("INDICATOREND")
    f.close()


def read_indicators_from_ilist(ilist):
    """
    returns the list of indicators and their weights from the ilist
    :param ilist: 
    :return: list of indicators and weights present in ilist
    """
    indicators = []
    weights = []
    f = open(ilist, 'r')
    for line in f:
        tokens = line.strip().split()
        if len(tokens) > 0 and tokens[0] == "INDICATOR":
            end_indx = len(tokens)
            for j in range(len(tokens)):
                if tokens[j][0] == '#':
                    end_indx = j
                    break
            indicator = ' '.join(x for x in tokens[2:end_indx])
            weights.append(float(tokens[1].split(":")[0]))
            indicators.append(indicator)

    f.close()
    return indicators, weights
