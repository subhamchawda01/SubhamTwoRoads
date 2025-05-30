#!/usr/bin/env python
import sys
import os

if len(sys.argv) < 2:
    print("Usage: " + sys.argv[0] + " Strat_file")


def rollValid(line):
    tokens = line.split(':')
    if len(tokens) == 2:
        if not os.path.isfile(tokens[1]):
            print("Error: File not present. Line " + line)
            sys.exit(8)
            # else: Check all subsequent file present
            #Ignore lines starting with #
    else:
        print("Error: Too many tokens. Line: " + line)


strat_file = sys.argv[1]
if not os.path.isfile(strat_file):
    print("Error: Strat File not present. Line " + strat_file)
    sys.exit(9)

strat_lines = []
with open(strat_file) as f:
    strat_lines = f.read().splitlines()

# Check file is in text form
if len(strat_lines) == 0:
    print("Error: No Lines present.")
    sys.exit(6)

start_line = strat_lines[0]
tokens = start_line.split()
start = 0
structured = 0

if len(tokens) == 0:
    print("LineNo: 0 Error: Blank Line")
    sys.exit(10)

if tokens[0] == 'STRUCTURED_TRADING':
    if len(tokens) < 4:  # Is it gauranteed?
        print("LineNo: 0 Error: No of tokens should be atleast 4. Line: " + start_line)
        sys.exit(1)
    elif not os.path.isfile(tokens[3]):
        print("LineNo: 0 Error: Structured trading file doesnot exists. Line: " + start_line)
        sys.exit(2)
    else:
        start = 1
        structured = 1
elif tokens[0] == 'STRUCTURED_STRATEGYLINE':
    if len(tokens) < 2:
        print("LineNo: 0 Error: No of tokens should be atleast 2. Line: " + start_line)
        sys.exit(1)
    elif not os.path.isfile(tokens[1]):
        print("LineNo: 0 Error: Structured strategyline file doesnot exists. Line: " + start_line)
        sys.exit(2)
    start = 1
    structured = 1

for num in range(start, len(strat_lines)):
    tokens = strat_lines[num].split()
    # print tokens
    if len(tokens) == 0:
        print("LineNo: " + str(num) + " Error: Blank Line")
        sys.exit(10)
    if structured == 1:
        if len(tokens) < 4:
            print("LineNo: " + str(num) + " Error: No of tokens should be atleast 4. Line: " + strat_lines[num])
            sys.exit(1)
        if not os.path.isfile(tokens[2]):
            print("LineNo: " + str(num) + " Error: Model file not present. Line: " + strat_lines[num])
            sys.exit(3)
        if tokens[3].split(':')[0] == 'ROLL':
            rollValid(tokens[3])
        elif not os.path.isfile(tokens[3]):
            print("LineNo: " + str(num) + " Error: Param file not present. Line: " + strat_lines[num])
            sys.exit(4)
    elif tokens[0] == 'STRATEGYLINE' or tokens[0] == 'PORT_STRATEGYLINE':
        if len(tokens) < 5:
            print("LineNo: " + str(num) + " Error: No of tokens should be atleast 5. Line: " + strat_lines[num])
            sys.exit(1)
        if tokens[1] == 'OptionsTrading':
            break
        if not os.path.isfile(tokens[3]):
            print("LineNo: " + str(num) + " Error: Model file not present. Line: " + strat_lines[num])
            sys.exit(3)
        if tokens[4].split(':')[0] == 'ROLL':
            rollValid(tokens[4])
        elif not os.path.isfile(tokens[4]):
            print("LineNo: " + str(num) + " Error: Param file not present. Line: " + strat_lines[num])
            sys.exit(4)
    else:
        print("LineNo: " + str(num) + " Error: Invalid Line. Line: " + strat_lines[num])
        sys.exit(5)

print("Valid Strat File")
sys.exit(0)
# print strat_lines
