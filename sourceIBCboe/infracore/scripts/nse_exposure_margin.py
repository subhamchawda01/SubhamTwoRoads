#!/usr/bin/env python

import sys
from datetime import datetime
from time import strptime

def Usage():
    print "============================USAGE==============================================================="
    print "Option 1: " + sys.argv[0] + " <YYYYMMDD>" 
    print "Assumes files exist at :"
    print "Bhavcopy: /spare/local/tradeinfo/NSE_Files/BhavCopy/fo/MMYY/DD/fo01JAN2015bhav.csv"
    print "Margins: /spare/local/tradeinfo/NSE_Files/ExposureMargins/Margin_YYYYMM.csv"
    print "Lotsizes: /spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_YYYYMMDD.csv"
    print "================================================================================================"
    print "Option 2: " + sys.argv[0] + " <path_to_bhavcopy_file> <path_to_margin_file> [<path_to_lot_size_file>]"
    print "If <path_to_lot_size_file> is not provided, it will report margin per unit size"
    print "================================================================================================"
    

def ParseBhavCopyFile(filename):
    product_to_price_map = {}
    lines = filter(lambda line: line !='',open(filename).read().split('\n'))
    for line in lines:
        try:
            tokens = line.split(',')
            instrument = tokens[0].strip()
            symbol = tokens[1].strip()
            expiry = datetime.strptime(tokens[2].strip(), "%d-%b-%Y")
            strike = float(tokens[3].strip())
            option_type = tokens[4].strip()
            price = float(tokens[9].strip())
            product_to_price_map[(instrument,symbol,expiry,strike,option_type)] = price
        except Exception as exception:
            pass
#             print "BhavCopyFile: " + line
    return product_to_price_map

def ParseMarginFile(filename):
    symbol_to_margin_percentage_map = {}
    lines = filter(lambda line: line !='', open(filename).read().split('\n'))
    for line in lines:
        try:
            tokens = line.split(',')
            symbol = tokens[0].strip()
            percentage = float(tokens[1].strip())
            symbol_to_margin_percentage_map[symbol] = percentage
        except Exception as exception:
            pass
#             print exception
#             print "MarginFile: " + line
    return symbol_to_margin_percentage_map

def ParseLotSizeFiles(filename):
    product_to_lotsize_map = {}
    lines = filter(lambda line: line !='',open(filename).read().split('\n'))
    expiries = []
    i = 0
    for line in lines:
        tokens = line.split(',')
        if ( i == 0 ):
            filtered_columns = [column.strip().lower() for column in tokens[2:]]
            filtered_columns = filter(lambda x: x!='', filtered_columns)
            expiries = [datetime.strptime(column.strip().lower(), "%b-%y") for column in filtered_columns]
            i += 1
        else:
            symbol = tokens[1].strip()
            j = 0
            filtered_columns = [column.strip() for column in tokens[2:]]
            filtered_columns = filter(lambda x: x!='', filtered_columns)
            for lotsize in filtered_columns:
                try:
                    product_to_lotsize_map[(symbol,expiries[j])] = int(lotsize)
                except:
                    product_to_lotsize_map[(symbol,expiries[j])] = 0
                j += 1
        i += 1
    return product_to_lotsize_map

def GetPrice(product_to_price_map, product):
    instrument = product[0]
    if ( "FUT" in instrument):
        return product_to_price_map[product]
    elif( "OPT" in instrument):
        instrument_ = "FUT" + instrument[3:]
        price = -1.0
        try:
            price = product_to_price_map[(instrument_,product[1],product[2],0.0,"XX")]
        except:
            #  Loop over all products prices to find            
            for key in product_to_price_map.keys():
                if ( "FUT" in key[0] and product[1] == key[1] ):
                    price = product_to_price_map[key]
    return price

            
        

def ComputeMarginsPerUnitSize(product_to_price_map, symbol_to_margin_percent_map):
    product_to_margin_value_map = {}
    for key in product_to_price_map.keys():
        instrument = key[0]
        symbol = key[1]
        
        price = GetPrice(product_to_price_map,key)
        
        if("IDX" in instrument):
            product_to_margin_value_map[key] = (3.0*price)/100.0 # 3% of Indexes 
        elif ("FUT" in instrument or "OPT" in instrument):
            try:
                margin_percent = symbol_to_margin_percent_map[symbol]
                product_to_margin_value_map[key] = (margin_percent*price)/100.0
            except:
#                 print symbol + "Not Found"
                product_to_margin_value_map[key] = (5*price)/100.0
        else:
            print "Unknown Instrument Type: " + instrument
    return product_to_margin_value_map

def ComputeMarginsPerLotSize(product_to_price_map, symbol_to_margin_percent_map, product_to_lotsize_map):
    product_to_margin_per_lotsize = {}
    product_to_margin_per_unit_size = ComputeMarginsPerUnitSize(product_to_price_map, symbol_to_margin_percent_map)
    
    for key in product_to_margin_per_unit_size.keys():
        symbol = key[1]
        expiry = datetime.strptime(key[2].strftime("%b-%y"),"%b-%y")
        lotsize = -1
        try:
            lotsize = product_to_lotsize_map[(symbol,expiry)]
        except:
            lotsize = -1
        product_to_margin_per_lotsize[key] = product_to_margin_per_unit_size[key] * lotsize
    
    return product_to_margin_per_lotsize

def OutputMarginFile(product_to_margin_value):
    for key in product_to_margin_value.keys():
        instrument, symbol, expiry, strike, option_type = key
        margin_val = product_to_margin_value[key]
        expiry = expiry.strftime("%d-%b-%y")
        print instrument, symbol, expiry, strike, option_type, margin_val
    
    
if (len(sys.argv) == 2):
    # Get the file names and execute
#     print "Option 1 not supported currently, Please try option 2 for now :)"
    YYYYMMDD = sys.argv[1]
    text_format_date = datetime.strptime(YYYYMMDD,"%Y%m%d").strftime("%d%b%Y").upper()
    bhavcopy_filename = "/spare/local/tradeinfo/NSE_Files/BhavCopy/fo/" + YYYYMMDD[4:6] + YYYYMMDD[2:4] + "/fo" + text_format_date + "bhav.csv"
    margin_percentage_filename = "/spare/local/tradeinfo/NSE_Files/Margin_" + YYYYMMDD +".csv"
    lotsize_filename = "/spare/local/tradeinfo/NSE_Files/Lotsizes/fo_mktlots_" + YYYYMMDD + ".csv"
    product_to_price_map = ParseBhavCopyFile(bhavcopy_filename)
    symbol_to_margin_percent_map = ParseMarginFile(margin_percentage_filename)
    product_to_lotsize_map = ParseLotSizeFiles(lotsize_filename)
    product_to_margin_per_lotsize = ComputeMarginsPerLotSize(product_to_price_map, symbol_to_margin_percent_map, product_to_lotsize_map)
    OutputMarginFile(product_to_margin_per_lotsize)
    
elif ( len(sys.argv) == 4):
    product_to_price_map = ParseBhavCopyFile(sys.argv[1])
    symbol_to_margin_percent_map = ParseMarginFile(sys.argv[2])
    product_to_lotsize_map = ParseLotSizeFiles(sys.argv[3])
    product_to_margin_per_lotsize = ComputeMarginsPerLotSize(product_to_price_map, symbol_to_margin_percent_map, product_to_lotsize_map)
    OutputMarginFile(product_to_margin_per_lotsize)
    
elif ( len(sys.argv) == 3):
    product_to_price_map = ParseBhavCopyFile(sys.argv[1])
    symbol_to_margin_percent_map = ParseMarginFile(sys.argv[2])
    product_to_margin_per_unitsize = ComputeMarginsPerUnitSize(product_to_price_map, symbol_to_margin_percent_map)
    OutputMarginFile(product_to_margin_per_unitsize)
else:
    Usage()
    exit()
    
    
    
                    
                    
            
        
    
    
    
    