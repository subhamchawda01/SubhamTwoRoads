import sys
import csv
import datetime

'''
Run this file when there are discrepancies in trade audit log file due to any crashes
./trade_file_corrector.py exch_trade.csv final_audit.csv

Make sure that the exch_trade file end in csv format

What it simply does is, checks if there are any trades missing in final_audit.csv and appends
it to the end.


It accomplishes this by defining a mapping between the two files (defined in ExchToDVCConverter)
and using that to write out the missing trades in a converted format.

Code is pretty much self explanatory. Start from main()

'''



class BaseTradeFile(object):
  def get_key_value_dict(self, values_list):
    k_v_dict = {}
    index = 0
    for tup in self._trade_dict:
      if index >= len(self._trade_dict) or index >= len(values_list):
        break
      field = tup[0]
      if field is not None:
        k_v_dict[field] = values_list[index]
        # print (field, values_list[index])
      index += 1

    return k_v_dict

class ExchangeTradeFile(BaseTradeFile):
  # Create the dict file with default values for each field.
  # Protected member; Should be used only when inherited.
  # Make sure to list down in the order of csv headers
  @staticmethod
  def get_delimiter():
    return '\t'

  def __init__(self):
    self._trade_dict = [
      ('trade_no', 0),
      ('trade_status', 11), # 11 - Original Trade
      ('instrument_type', ''),
      ('symbol', ''),
      ('expiry_date', ''), # Using start of unix time as default expiry date.
      ('strike_price', ''),
      ('option_type', 'XX'), # Default - FO. Since problems seem to be coming in FO
      ('security_name', ''),
      ('book_type', 1),
      ('book_type_name', 1),
      ('user_id', ''),
      ('market_type', 1),
      #('branch_no', ''),
      ('trade_type_buy_sell', ''),
      ('trade_qty', 0),
      ('trade_price', 0.0),
      ('pro_or_client', 2), # Pro
      ('account_number', ''),
      ('participant', ''),
      ('open_flag', ''),
      #('cover_flag', ''),
      ('activity_time', ''),
      ('last_modified', ''),
      ('order_no', ''),
      ('opposite_broker_id', ''),
      ('modified_date_time', '')
    ]

  def get_unique_code(self, from_values):
    return from_values['trade_no']


class DVCTradeFile(BaseTradeFile):
  # Create the dict file with default values for each field.
  # Protected member; Should be used only when inherited.
  # Make sure to list down in the order of csv headers
  @staticmethod
  def get_delimiter():
    return ','

  def __init__(self):
    self._trade_dict = [
      ('txn_code', 20222),
      ('log_time', ''),
      ('trader_id', ''),
      ('timestamp', ''),
      ('timestamp_1', ''),
      ('timestamp_2', ''),
      ('order_num', ''), # Default - FO. Since problems seem to be coming in FO
      ('broker', 90044),
      ('account', 90044),
      ('buy_sell', ''),
      ('orig_vol', 0),
      ('dis_vol', 0),
      ('rem_vol', 0),
      ('dq_rem_vol', 0),
      ('price', 0.0),
      ('gtd', 0),
      ('fill_number', ''),
      ('fill_qty', ''),
      ('fill_price', ''),
      ('vol_filled', ''),
      ('activity_type', ''),
      ('activity_time', ''),
      ('token', ''),
      ('instrument', ''),
      ('underlying', ''),
      ('expiry_date', ''),
      ('strike_px', -1),
      ('option_Type', 'XX'),
      ('oc_flag', 'O'), # Open/Close flag
      ('book_type', 1),
      ('participant', 90044),
      ('trade_value', self.calculate_trade_value),
    ]

  def get_unique_code(self, from_values):
    return from_values['fill_number']

  def get_product_information(self, from_values):
    return from_values['underlying']

  def get_token(self, from_values):
    return from_values['token']

  def get_timestamp_2(self, from_values):
    return from_values['timestamp_2']

  def calculate_trade_value(self, dict_values):
    return int(dict_values['fill_qty']) * float(dict_values['fill_price'])


class ExchToDVCConverter(ExchangeTradeFile, DVCTradeFile):
  def __init__(self):
    exchange = ExchangeTradeFile()
    dvc_trade = DVCTradeFile()
    self.from_field_array = exchange._trade_dict
    self.to_field_array = dvc_trade._trade_dict

    # Specify fields which have a direct one to one mapping
    # Specify in the order of to -> from.
    self.direct_field_mapping = {
      'broker': 'account_number',
      'account': 'account_number',
      'participant': 'account_number',
      'buy_sell': 'trade_type_buy_sell',
      'fill_number': 'trade_no',
      'fill_qty': 'trade_qty',
      'orig_vol': 'trade_qty',
      'vol_filled': 'trade_qty',
      'fill_price': 'trade_price',
      'price': 'trade_price',
      'underlying': 'instrument_type',
      'instrument': 'symbol',
      'trader_id': 'user_id'
    }

    # Specify fields which need a converter function
    self.converter_field_mappings = {
      'expiry_date': self.convert_expiry_date,
      'log_time': self.get_timestamp,
      'timestamp': self.get_hypheneted_timestamp,
      'timestamp_1': self.get_hypheneted_timestamp,
      'activity_time': self.get_timestamp,
      'activity_type': self.get_buy_sell_symbol,
    }

    # Rest will be converted using the default values.

  def convert_expiry_date(self, from_dict, to_dict={}):
    from_date_format = "%d-%b-%y"
    to_date_format = "%Y%m%d"
    date = datetime.datetime.strptime(from_dict['expiry_date'], from_date_format).strftime(to_date_format)
    return date

  def get_timestamp(self, from_dict):
    from_date_format = "%d-%m-%Y %H:%M"
    to_date_format = "%Y%m%d-%H:%M:00"

    date = datetime.datetime.strptime(from_dict['activity_time'], from_date_format).strftime(to_date_format)
    return date

  def get_hypheneted_timestamp(self, from_dict):
    from_date_format = "%d-%m-%Y %H:%M"
    to_date_format = "%Y-%m-%d %H:%M:00.000000"

    date = datetime.datetime.strptime(from_dict['activity_time'], from_date_format).strftime(to_date_format)
    return date

  def get_buy_sell_symbol(self, from_dict):
    return 'B' if int(from_dict['trade_type_buy_sell']) == 1 else 'S'


  def convert(self, from_values, to_values={}):
    # from_values and to_values are dicts with above mentioned fields.
    for field, default_method in self.to_field_array:
      if field in self.direct_field_mapping:
        field_mapping = self.direct_field_mapping[field]
        to_values[field] = from_values[field_mapping].strip()
      elif field in self.converter_field_mappings:
        to_values[field] = self.converter_field_mappings[field](from_values)
      else:
        if callable(default_method):
          to_values[field] = default_method(to_values)
        else:
          to_values[field] = default_method

    return to_values


def process_extra_fields(target_dict, token_dict, timestamp_dict):
  if target_dict['token'] == '':
    if target_dict['underlying'] in token_dict:
      target_dict['token'] = token_dict[target_dict['underlying']]

  if target_dict['timestamp_2'] == '':
    if target_dict['underlying'] in timestamp_dict:
      target_dict['timestamp_2'] = timestamp_dict[target_dict['underlying']]

  return target_dict


def convert_to_array(target_dict, to_field_array):
  # Convert to a list as mentioned in to_field_array
  values = []
  for field, default_method in to_field_array:
    values += [str(target_dict[field])]

  return values


def main():
  if len(sys.argv) != 3:
    print ("Usage: ./script.py from_file.csv to_file.csv")
    exit(-1)

  from_filename = sys.argv[1]
  to_filename = sys.argv[2]

  exch_trade_file = ExchangeTradeFile()
  dvc_trade_file = DVCTradeFile()
  converter = ExchToDVCConverter()

  trade_dict = {}
  token_dict = {}
  timestamp_dict = {}


  with open(to_filename, mode='r') as infile:
    reader = csv.reader(infile, delimiter=DVCTradeFile.get_delimiter())
    for row in reader:
      from_values = dvc_trade_file.get_key_value_dict(row)
      trade_dict[dvc_trade_file.get_unique_code(from_values)] = 1

      # The next 2 information fields remains constant for a given underlying. Can be extracted from existing data.
      # This can be manually confirmed by looking through the data file too.
      # These are required since the source file for correction doesn't have these values
      # and we have to manually recreate them from the existing data.

      token_dict[dvc_trade_file.get_product_information(from_values).strip()] = dvc_trade_file.get_token(from_values)
      timestamp_dict[dvc_trade_file.get_product_information(from_values).strip()] = dvc_trade_file.get_timestamp_2(from_values)

  with open(to_filename, mode='a') as outfile:
    with open(from_filename, mode='r') as infile:
      reader = csv.reader(infile, delimiter=ExchangeTradeFile.get_delimiter())
      for row in reader:
          from_values = exch_trade_file.get_key_value_dict(row)
          unique_code = exch_trade_file.get_unique_code(from_values)
          if unique_code not in trade_dict:
            to_values = converter.convert(from_values)
            to_values = process_extra_fields(to_values, token_dict, timestamp_dict)
            to_values = convert_to_array(to_values, converter.to_field_array)
            outfile.write(",".join(to_values))

main()