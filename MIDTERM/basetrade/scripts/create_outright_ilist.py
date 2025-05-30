#!/usr/bin/env python

"""
Run the compute model-param for a given config

"""

import argparse


def create_ilist_from_template(template, shortcode):
    """
        
    :param template: 
    :param shortcode: 
    :return: 
    """
    template_content = open(template).readlines()
    template_content = [line for line in template_content if line[0] != '#']
    template_content = [line.split() for line in template_content]
    template_content = [line for line in template_content if len(line)>0 ]

    shc_split = shortcode.split('_')
    expiry = shc_split[-1]

    for iter in range(len(template_content)):
        if template_content[iter][0] == 'MODELINIT':
            template_content[iter][2] = shortcode
        elif template_content[iter][0] == 'INDICATOR':
            template_content[iter] = ' '.join(template_content[iter])
            modified_string = replace_expiries(template_content[iter], expiry)
            if not modified_string:
                template_content[iter] = []
            else:
                template_content[iter] = modified_string.split()

        elif template_content[iter][0] == 'MODELMATH' or template_content[iter][0] == 'INDICATORSTART' \
                or template_content[iter][0] == 'INDICATOREND':
            continue

    # print "Final Ilist"
    for line in template_content:
        if line:
            print(' '.join(line))


def replace_expiries(indicator_line, expiry):
    # print(indicator_line)
    """
    Replace the expiry iterator with exact value given this shortcode
    
    :param indicator_line: 
    :param expiry: 
    :return:
     
    """

    indicator_line = indicator_line.replace('$i', expiry)

    while True:
        idx = indicator_line.find('$(i') + 3  # to account for search string

        if idx > 2:
            end_idx = indicator_line[idx:].find(')')
            int_num = int(indicator_line[idx:idx+end_idx])
            # evaluate the correct expiry
            this_expiry = int(expiry) + int_num

            if 0 <= this_expiry < 15:
                # replace the '$(i+n)' with appropriate integer number
                indicator_line = indicator_line.replace(indicator_line[idx-3:idx+3],str(this_expiry))
            else:
                return ""
        else:
            break
            
    return indicator_line


parser = argparse.ArgumentParser()
parser.add_argument('-t', dest='templatename', help="template ilist for product", type=str, required=True)
parser.add_argument('-s', dest='shortcode', help='shortcode to create ilist for', type=str, required=True)

args = parser.parse_args()

create_ilist_from_template(args.templatename, args.shortcode)