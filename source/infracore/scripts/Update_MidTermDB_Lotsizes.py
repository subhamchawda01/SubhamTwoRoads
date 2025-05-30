#!/usr/bin/python
# -*- coding: utf-8 -*-
import os
import sys
import sqlite3
import argparse
import pandas as pd

CONTRACT_FILE_DIR = '/spare/local/tradeinfo/NSE_Files/ContractFiles/'


def LoadLotSizeInfoFromContractFile(file, date):
    df = pd.read_csv(
        file,
        delim_whitespace=True,
        header=None,
        names=['Type', 'Underlying', 'Lotsize', 'Expiry'],
        usecols=[0, 1, 3, 5],
        comment='#',
        )
    df = df[(df['Type'] == 'STKFUT') | (df['Type'] == 'IDXFUT')]
    df = df[df['Expiry'] >= int(date)]
    df = df.drop_duplicates()
    expiries = sorted(set(df['Expiry'].tolist()))
    exp_num_map = {
    	expiries[i] : i for i in range(0,len(expiries))
    }
    df['Exp_num'] = df['Expiry'].apply(lambda x: exp_num_map[x])
    del df['Type']
    return df


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--date')
    parser.add_argument('--db_path')
    args = parser.parse_args()
    if args.date:
        date = args.date
    else:
        print('--date argument required, Exiting...')
        exit(0)
    if args.db_path and os.path.isfile(args.db_path):
        db_path = args.db_path
    else:
        print('--db_path argument required, Exiting...')
        exit(0)
    contract_file = CONTRACT_FILE_DIR + 'nse_contracts.' + date
    if not os.path.isfile(contract_file):
        print('Contract file -> %s, Not exists, Exiting...' \
            % contract_file)
        exit(0)
    df = LoadLotSizeInfoFromContractFile(contract_file, date) 
    conn = sqlite3.connect(db_path)
    for (key, value) in df.iterrows():
        stmt = \
            "insert into LOTSIZES(day, stock, expnum, expiry, lotsize) values (%s, \'%s\', %d, %d, %d);" \
            % (date, value['Underlying'], value['Exp_num'],
               value['Expiry'], value['Lotsize'])
        conn.execute(stmt);

    conn.commit()


if __name__ == '__main__':
    main()
