import os, sys, subprocess
import argparse
import datetime

underlyings = ["PNB", "USDINR"]
products = ["FUT"]
test_dates = []

def diff_files(filename1, filename2, print_diff):
  # filename1 is the source file, which will be checked in filename2
  f = open(filename1, "r")
  lines = f.readlines()

  current_time = ""
  time_map = {}

  for line in lines:
    if line.startswith("Timestamp"):
      current_time = line.split()[1]
      time_map[current_time] = {}
    elif line.startswith("Best bid BUY"):
      time_map[current_time]['BUY'] = {'price': line.split()[4], 'size': line.split()[7]}
    elif line.startswith("Best bid SELL"):
      time_map[current_time]['SELL'] = {'price': line.split()[4], 'size': line.split()[7]}

  last_timestamp = current_time

  f = open(filename2, "r")
  lines = f.readlines()

  try:
    for line in lines:
      if line.startswith("Timestamp"):
        current_time = line.split()[1]
      elif line.startswith("Best bid BUY"):
        if current_time in time_map:
          price = line.split()[4]
          size = line.split()[7]
          if time_map[current_time]['BUY']['price'] == price:
            del time_map[current_time]['BUY']['price']
          if time_map[current_time]['BUY']['size'] == size:
            del time_map[current_time]['BUY']['size']

          if current_time in time_map and len(time_map[current_time]['BUY']) == 0:
            del time_map[current_time]['BUY']
      elif line.startswith("Best bid SELL"):
        if current_time in time_map:
          price = line.split()[4]
          size = line.split()[7]
          if time_map[current_time]['SELL']['price'] == price:
            del time_map[current_time]['SELL']['price']
          if time_map[current_time]['SELL']['size'] == size:
            del time_map[current_time]['SELL']['size']

          if current_time in time_map and len(time_map[current_time]['SELL']) == 0:
            del time_map[current_time]['SELL']

      if current_time in time_map and len(time_map[current_time]) == 0:
        del time_map[current_time]
  except Exception as e:
    print ("Exception at time: ", current_time)
    print (e)


  if print_diff:
    f = open(filename1 + "-diff.txt", "w")
    for k, v in time_map.items():
      f.write(k,v)

  converges = False
  if last_timestamp not in time_map:
    converges = True

  return len(time_map), converges


def daterange(start_date_string, end_date_string):
  start_date = datetime.datetime.strptime(start_date_string, "%Y%m%d").date()
  end_date = datetime.datetime.strptime(end_date_string, "%Y%m%d").date()
  for n in range(int((end_date - start_date).days) + 1):
    yield start_date + datetime.timedelta(n)

def parse_arguments():
  global test_dates
  global underlyings
  global products

  parser = argparse.ArgumentParser()
  parser.add_argument("-d", "--diff", action="store_true")
  parser.add_argument("-do", "--diff_only", action="store_true")
  parser.add_argument("-u", "--underlyings", nargs = '*', help="Underlying values. Eg. 'PNB', 'USDINR'", default=["PNB", "USDINR"])
  parser.add_argument("-p", "--products", nargs = '*', help="Product values. Eg. 'FUT'", default=["FUT"])
  parser.add_argument("-D", "--date", help="Single Date. Eg. 20130104", default="20170222")
  parser.add_argument("-Dr", "--date_range", nargs = 2, help="Date Range. Eg. 20130104 20170101")
  parser.add_argument("-df", "--diff_file_names", nargs = 2, help="Diff files. Eg. PNB-FUT0-20170222-o.txt PNB-FUT0-20170222-n.txt")
  args = parser.parse_args()
  '''
  print (args.products)
  print (args.underlyings)
  print (args.date_range)
  '''

  if args.underlyings:
    underlyings = args.underlyings

  if args.products:
    products = args.products

  if args.date:
    start_date, end_date = args.date, args.date

  if args.date_range:
    start_date, end_date = args.date_range[0], args.date_range[1]

  for date in daterange(start_date, end_date):
      test_dates += [date.strftime('%Y%m%d')]

  return args

def parse_product_info(product_info):
  if product_info == 'FUT':
    return 'FUT0'


def main():
  args = parse_arguments()
  if args.diff_only:
    if not args.diff_file_names:
      print ("Specify file names with -df flag")
      exit(-1)

    filename1 = args.diff_file_names[0]
    filename2 = args.diff_file_names[1]
    diff, converges = diff_files(filename1, filename2, args.diff)
    print (diff, converges)
    exit(0)

  for underlying in underlyings:
    for product_info in products:
      for date in test_dates:
        parsed_product_info = parse_product_info(product_info)
        original_filename = "{0}-{1}-{2}-o.txt".format(underlying, parsed_product_info, date)
        mkt_filename = "{0}-{1}-{2}-n.txt".format(underlying, parsed_product_info, date)

        print (date, underlying, product_info)
        original_command = "../bin/original_trade_logger SIM NSE_{0}_{1} {2} > {3}".format(underlying, parsed_product_info, date, original_filename)
        mkt_command = "../bin/mkt_trade_logger SIM NSE_{0}_{1} {2} > {3}".format(underlying, parsed_product_info, date, mkt_filename)
        rm_command = "rm {0} {1}".format(original_filename, mkt_filename)

        subprocess.check_output(mkt_command, shell=True)
        output = subprocess.check_output(original_command, shell=True)

        diff, converges = diff_files(original_filename, mkt_filename, args.diff)
        print (diff, converges)
        if converges:
          subprocess.check_output(rm_command, shell=True)


main()

